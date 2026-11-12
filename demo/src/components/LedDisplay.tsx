export function LedDisplay({
  label,
  shorthand,
  value,
  onValueChange,
}: {
  label: string;
  shorthand: string;
  value: number;
  onValueChange: (newValue: number) => void;
}) {
  return (
    <div className="bg-black/40 w-full h-20 rounded mb-2 flex items-center justify-center border border-zinc-900/50 relative">
      <input
        className="font-mono text-7xl text-red-600 font-bold drop-shadow-[0_0_8px_rgba(220,38,38,0.6)] tracking-widest w-full text-center"
        value={value}
        onChange={(e) => onValueChange(parseFloat(e.target.value))}
        type="number"
      />
      <div className="absolute -right-5 top-2 flex flex-col text-[10px] text-zinc-600 gap-2 font-bold">
        <span>%</span>
        <span>{shorthand}</span>
      </div>
      <p className="absolute left-0 right-0 bottom-0 text-center text-gray-400 font-bold text-[10px]">
        {label}
      </p>
    </div>
  );
}
