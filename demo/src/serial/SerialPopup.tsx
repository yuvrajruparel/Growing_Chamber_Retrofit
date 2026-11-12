import { useState, useRef, useEffect } from "react";
import type { UseSerialInterfaceReturn } from "./interface";
import { Wifi, WifiOff, Terminal, Trash2, X } from "lucide-react";

interface SerialMonitorProps {
  serial: UseSerialInterfaceReturn;
}

export function SerialMonitor({ serial }: SerialMonitorProps) {
  const [isOpen, setIsOpen] = useState(false);
  const logsEndRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    if (isOpen) {
      logsEndRef.current?.scrollIntoView({ behavior: "smooth" });
    }
  }, [serial.logs, isOpen]);

  return (
    <>
      <div className="fixed bottom-6 right-6 z-50">
        <button
          onClick={() => setIsOpen(!isOpen)}
          className={`shadow-xl flex items-center gap-2 px-4 py-3 rounded-full font-bold transition-all hover:scale-105 active:scale-95 ${
            serial.isConnected
              ? "bg-emerald-600 text-white hover:bg-emerald-500 shadow-emerald-900/20"
              : "bg-slate-700 text-slate-300 hover:bg-slate-600"
          }`}
        >
          {serial.isConnected ? (
            <Wifi className="w-5 h-5" />
          ) : (
            <WifiOff className="w-5 h-5" />
          )}
          {isOpen ? "Close Monitor" : "Serial Monitor"}
          {serial.isConnected && (
            <span className="absolute -top-1 -right-1 flex h-3 w-3">
              <span className="animate-ping absolute inline-flex h-full w-full rounded-full bg-emerald-400 opacity-75"></span>
              <span className="relative inline-flex rounded-full h-3 w-3 bg-emerald-500"></span>
            </span>
          )}
        </button>
      </div>

      <div
        className={`fixed bottom-0 left-0 right-0 z-40 bg-slate-900 border-t border-slate-700 shadow-2xl transition-transform duration-300 ease-in-out transform ${
          isOpen ? "translate-y-0" : "translate-y-full"
        }`}
        style={{ height: "50vh" }}
      >
        <div className="flex items-center justify-between px-6 py-3 bg-slate-800 border-b border-slate-700">
          <div className="flex items-center gap-4">
            <h3 className="font-mono font-bold text-slate-200 flex items-center gap-2">
              <Terminal className="w-4 h-4 text-blue-400" />
              /dev/ttyUSB*
            </h3>

            {!serial.isConnected ? (
              <button
                onClick={serial.connect}
                className="text-xs bg-blue-600 hover:bg-blue-500 text-white px-3 py-1.5 rounded-md font-medium transition-colors"
              >
                Connect Device
              </button>
            ) : (
              <button
                onClick={serial.disconnect}
                className="text-xs bg-red-900/30 text-red-400 border border-red-900 hover:bg-red-900/50 px-3 py-1.5 rounded-md font-medium transition-colors"
              >
                Disconnect
              </button>
            )}

            <span className="text-xs text-slate-500 hidden sm:inline-block">
              Last State:{" "}
              <span className="text-yellow-500">
                {JSON.stringify(serial.data)}
              </span>
            </span>
          </div>

          <div className="flex items-center gap-2">
            <button
              onClick={serial.clearLogs}
              className="p-2 hover:bg-slate-700 rounded-md text-slate-400 hover:text-white"
              title="Clear Logs"
            >
              <Trash2 className="w-4 h-4" />
            </button>
            <button
              onClick={() => setIsOpen(false)}
              className="p-2 hover:bg-slate-700 rounded-md text-slate-400 hover:text-white"
            >
              <X className="w-5 h-5" />
            </button>
          </div>
        </div>

        <div className="h-[calc(50vh-60px)] overflow-y-auto bg-black p-4 font-mono text-xs space-y-1">
          {serial.logs.length === 0 && (
            <div className="text-slate-600 text-center mt-10">
              Waiting for serial data...
            </div>
          )}
          {serial.logs.map((log, i) => (
            <div key={i} className="flex gap-3 hover:bg-white/5 p-0.5 rounded">
              <span className="text-slate-600 shrink-0 select-none">
                [{log.timestamp}]
              </span>
              <span
                className={`${
                  log.type === "rx"
                    ? "text-cyan-400"
                    : log.type === "tx"
                      ? "text-emerald-400"
                      : log.type === "error"
                        ? "text-red-400"
                        : log.type === "success"
                          ? "text-green-400"
                          : "text-slate-300"
                }`}
              >
                {log.type === "rx" ? "<< " : log.type === "tx" ? ">> " : ""}
                {log.msg}
              </span>
            </div>
          ))}
          <div ref={logsEndRef} />
        </div>
      </div>
    </>
  );
}
