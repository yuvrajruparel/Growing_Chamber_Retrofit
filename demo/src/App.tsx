import { useState } from "react";
import { Screw } from "./components/Screw";
import { LedIndicator } from "./components/LedIndicator";
import { StatusLight } from "./components/StatusLight";
import { LedDisplay } from "./components/LedDisplay";
import { useSerialInterface } from "./serial/interface";
import { SerialMonitor } from "./serial/SerialPopup";

export default function App() {
  const serial = useSerialInterface();
  const [humidity, setRealHumidity] = useState(50);
  const [setpoint, setSetpoint] = useState(50);
  const mode = serial.isConnected ? serial.mode || "ONLINE" : "OFFLINE";

  const updateHumidity = (newHumidity: number) => {
    setRealHumidity(newHumidity);
    if (serial.isConnected) {
      serial.update({ actual: newHumidity, setpoint });
    }
  };

  const updateSetpoint = (newSetpoint: number) => {
    setSetpoint(newSetpoint);
    if (serial.isConnected) {
      serial.update({ actual: humidity, setpoint: newSetpoint });
    }
  };

  const handleUp = () => updateSetpoint(Math.min(setpoint + 1, 100));
  const handleDown = () => updateSetpoint(Math.max(setpoint - 1, 0));

  return (
    <div className="w-full h-screen bg-gray-300 flex items-center justify-center p-8 font-sans select-none overflow-hidden">
      <div className="absolute inset-0 opacity-10 bg-[url('https://www.transparenttextures.com/patterns/brushed-alum.png')] bg-repeat pointer-events-none"></div>

      {/* Main Black Panel Chassis */}
      <div className="relative w-[700px] h-[480px] bg-zinc-800 rounded-sm shadow-[0_10px_20px_rgba(0,0,0,0.5)] border-t border-zinc-700 flex flex-col p-4">
        {/* Screws */}
        <div className="absolute top-4 left-4">
          <Screw />
        </div>
        <div className="absolute top-4 right-4">
          <Screw />
        </div>
        <div className="absolute bottom-4 left-4">
          <Screw />
        </div>
        <div className="absolute bottom-4 right-4">
          <Screw />
        </div>

        {/* --- PANEL CONTENT --- */}
        <div className="flex flex-1 gap-6 p-6">
          {/* LEFT COLUMN: OUT Gauge & Indicators */}
          <div className="w-1/4 flex flex-col justify-between pt-8 pb-4">
            {/* The "OUT" Gauge Area */}
            <div className="border-2 border-white h-40 w-12 mx-auto flex flex-col items-center justify-between py-2 relative">
              <p className="text-red-900/60 text-4xl font-bold font-mono pt-2 leading-6">
                + <br /> -
              </p>
              <span className="text-gray-100 font-bold tracking-widest text-sm">
                OUT
              </span>
              <p className="text-red-900/60 text-4xl font-bold font-mono pt-2 leading-6">
                + <br /> -
              </p>
            </div>

            {/* Left Status LEDs */}
            <div className="pl-2 mt-8 flex flex-col gap-1">
              <LedIndicator label="RAMP" />
              <LedIndicator label="LOCK" />
              <LedIndicator label="DEFROST" />
              <LedIndicator label="ALARM" />
            </div>
          </div>

          {/* CENTER COLUMN: Displays */}
          <div className="w-2/4 flex flex-col items-center pt-6 text-center ">
            {/* TOP DISPLAY (Real Humidity) */}
            <LedDisplay
              label="ACTUAL HUMIDITY"
              shorthand="R"
              value={humidity}
              onValueChange={updateHumidity}
            />

            {/* SETPOINT DISPLAY */}
            <LedDisplay
              label="SETPOINT"
              shorthand="S"
              value={setpoint}
              onValueChange={updateSetpoint}
            />

            {/* 1-8 STATUS GRID & SSR */}
            <div className="w-full border-2 border-white p-1 flex justify-between items-center mb-6 h-14 relative">
              {/* 1-4 Top Row */}
              <div className="absolute top-2 left-2 flex gap-4">
                <StatusLight num="1" />
                <StatusLight num="2" />
                <StatusLight num="3" />
                <StatusLight num="4" />
              </div>
              {/* 5-8 Bottom Row */}
              <div className="absolute bottom-2 left-2 flex gap-4">
                <StatusLight num="5" />
                <StatusLight num="6" />
                <StatusLight num="7" />
                <StatusLight num="8" />
              </div>

              <div className="ml-auto pr-2 text-gray-100 font-bold text-sm">
                SSR
              </div>
            </div>

            {/* BOTTOM DISPLAY (Alphanumeric) */}
            <div className="bg-black/40 min-w-48 h-12 rounded flex items-center justify-center border border-zinc-900/50 w-fit px-4">
              <span className="font-mono text-3xl text-red-600 font-bold drop-shadow-[0_0_5px_rgba(220,38,38,0.5)]">
                {mode}
              </span>
            </div>
          </div>

          {/* RIGHT COLUMN: Buttons */}
          <div className="w-1/4 flex flex-col items-center justify-between pt-6 pb-4">
            {/* UP/DOWN BUTTONS */}
            <div className="flex flex-col gap-4 text-white text-2xl">
              <button
                onClick={handleUp}
                className="w-20 h-20 border-2 border-white bg-zinc-800 active:bg-zinc-900 flex items-center justify-center shadow-lg active:shadow-none active:translate-y-0.5 transition-all"
              >
                ▲
              </button>
              <button
                onClick={handleDown}
                className="w-20 h-20 border-2 border-white bg-zinc-800 active:bg-zinc-900 flex items-center justify-center shadow-lg active:shadow-none active:translate-y-0.5 transition-all"
              >
                ▼
              </button>
            </div>

            {/* LABEL */}
            <div className="text-center my-4">
              <h2 className="text-gray-100 text-3xl font-bold tracking-tight">
                EGC
              </h2>
              <h2 className="text-gray-300 text-2xl font-bold tracking-tight">
                TC2
              </h2>
            </div>

            {/* ENTER BUTTON */}
            <button className="w-24 h-16 border-2 border-white bg-zinc-800 active:bg-zinc-900 flex items-center justify-center shadow-lg active:shadow-none active:translate-y-0.5 transition-all mb-2">
              <span className="text-gray-100 font-bold text-lg">ENTER</span>
            </button>
          </div>

          {/* FAR RIGHT STATUS COLUMN (Nestled next to buttons) */}
          {/*<div className="flex flex-col justify-end pb-20 gap-1 ml-[-20px]">
            <LedIndicator label="RUN" active={mode === "RUN"} />
            <LedIndicator label="EDIT" active={mode === "EDIT"} />
            <LedIndicator label="MOD" />
          </div>*/}
        </div>
      </div>

      <SerialMonitor serial={serial} />
    </div>
  );
}
