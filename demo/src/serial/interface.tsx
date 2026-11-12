import { useState, useRef, useCallback } from "react";

// Data shape expected from MCU
export interface SerialData {
  setpoint?: number;
  actual?: number;
  state?: string;
  [key: string]: unknown;
}

export interface LogEntry {
  msg: string;
  type: "info" | "success" | "error" | "rx" | "tx";
  timestamp: string;
}

export interface UseSerialInterfaceReturn {
  connect: () => Promise<void>;
  disconnect: () => Promise<void>;
  update: (payload: Record<string, unknown>) => Promise<void>;
  clearLogs: () => void;
  isConnected: boolean;
  logs: LogEntry[];
  data: SerialData;
  mode: string;
}

// Helper types for Web Serial API
interface SerialPort {
  open(options: { baudRate: number }): Promise<void>;
  close(): Promise<void>;
  readable: ReadableStream<unknown> | null;
  writable: WritableStream<unknown> | null;
}

interface NavigatorSerial {
  serial: {
    requestPort(): Promise<SerialPort>;
  };
}

/**
 * Serial interface hook
 * @returns
 */
export function useSerialInterface(): UseSerialInterfaceReturn {
  const [isConnected, setIsConnected] = useState<boolean>(false);
  const [logs, setLogs] = useState<LogEntry[]>([]);
  const [data, setData] = useState<SerialData>({});

  // Refs for stream management
  const portRef = useRef<SerialPort | null>(null);
  const readerRef = useRef<ReadableStreamDefaultReader<string> | null>(null);
  const writerRef = useRef<WritableStreamDefaultWriter<string> | null>(null);

  // Keep track of closing state to prevent error loops
  const isClosingRef = useRef(false);

  const addLog = useCallback((msg: string, type: LogEntry["type"] = "info") => {
    setLogs((prev) => [
      ...prev.slice(-49),
      {
        msg,
        type,
        timestamp: new Date().toLocaleTimeString(),
      },
    ]);
  }, []);

  // Internal cleanup function to reset state/refs
  const internalDisconnect = useCallback(async () => {
    if (isClosingRef.current) return;
    isClosingRef.current = true;

    try {
      if (readerRef.current) {
        await readerRef.current.cancel();
        readerRef.current = null;
      }
      if (writerRef.current) {
        await writerRef.current.close();
        writerRef.current = null;
      }
      if (portRef.current) {
        await portRef.current.close();
        portRef.current = null;
      }
    } catch (e) {
      console.error("Cleanup error (expected):", e);
    } finally {
      setIsConnected(false);
      isClosingRef.current = false;
      addLog("Disconnected", "info");
    }
  }, [addLog]);

  const connect = async () => {
    const nav = navigator as unknown as NavigatorSerial;
    if (!nav.serial) {
      alert("Web Serial API not supported in this browser.");
      return;
    }

    if (isConnected) return;

    try {
      const port = await nav.serial.requestPort();
      await port.open({ baudRate: 115200 });

      portRef.current = port;
      setIsConnected(true);
      addLog("Connected to Serial Port", "success");

      // Setup Writer (Encoder)
      const textEncoder = new TextEncoderStream();
      if (port.writable) {
        textEncoder.readable.pipeTo(port.writable);
        writerRef.current = textEncoder.writable.getWriter();
      }

      // Setup Reader (Decoder)
      if (port.readable) {
        const textDecoder = new TextDecoderStream();
        // Pipe the binary stream to the text decoder
        port.readable.pipeTo(textDecoder.writable).catch((err) => {
          // This catches errors in the pipe itself (like device lost)
          console.error("Pipe error:", err);
        });

        const reader = textDecoder.readable.getReader();
        readerRef.current = reader;
        readLoop(reader);
      }
    } catch (err: unknown) {
      console.error(err);
      if (err instanceof Error) {
        addLog(`Connection failed: ${err.message}`, "error");
      } else {
        addLog(`Connection failed: ${String(err)}`, "error");
      }
      internalDisconnect();
    }
  };

  const disconnect = async () => {
    await internalDisconnect();
  };

  const readLoop = async (reader: ReadableStreamDefaultReader<string>) => {
    let buffer = "";
    try {
      while (true) {
        const { value, done } = await reader.read();
        if (done) break;
        if (value) {
          buffer += value;
          // Process line-by-line
          let eolIndex;
          while ((eolIndex = buffer.indexOf("\n")) >= 0) {
            const line = buffer.slice(0, eolIndex).trim();
            buffer = buffer.slice(eolIndex + 1);
            if (line) handleMessage(line);
          }
        }
      }
    } catch (error: unknown) {
      // If we are voluntarily closing, ignore the error
      if (!isClosingRef.current) {
        if (error instanceof Error) {
          addLog(`Read Error: ${error.message}`, "error");
          // CRITICAL: If device is lost, trigger full disconnect cleanup
          if (
            error.message.includes("lost") ||
            error.message.includes("network")
          ) {
            internalDisconnect();
          }
        } else {
          addLog(`Read Error: ${String(error)}`, "error");
        }
      }
    } finally {
      reader.releaseLock();
    }
  };

  const handleMessage = (line: string) => {
    try {
      addLog(`RX: ${line}`, "rx");

      // Try JSON first
      if (line.startsWith("{") && line.endsWith("}")) {
        const parsed = JSON.parse(line);
        setData((prev) => ({ ...prev, ...parsed }));
        return;
      }

      // Fallback to FSM Regex if JSON fails
      const matches = line.match(/FSM State: (.*)$/g);
      if (matches && matches[0]) {
        const state = matches[0].replace("FSM State: ", "").trim();
        setData((prev) => ({ ...prev, state }));
        return;
      }
    } catch (error: unknown) {
      addLog(`Parse Error: ${line}`, "error");
      console.error("Parse Error:", error);
    }
  };

  const update = async (payload: Record<string, unknown>) => {
    if (!writerRef.current) return;

    const str = JSON.stringify(payload) + "\n";
    try {
      await writerRef.current.write(str);
      addLog(`TX: ${JSON.stringify(payload)}`, "tx");
    } catch (e: unknown) {
      if (e instanceof Error) {
        addLog(`Send Error: ${e.message}`, "error");
      } else {
        addLog(`Send Error: ${String(e)}`, "error");
      }
    }
  };

  return {
    connect,
    disconnect,
    update,
    clearLogs: () => setLogs([]),
    isConnected,
    logs,
    data,
    mode: data.state || "UNKNOWN",
  };
}
