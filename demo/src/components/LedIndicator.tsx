// LED Indicator Component (Square LEDs)
// const LedIndicator = ({ label, active = false, color = "bg-red-600" }) => (
export function LedIndicator({
  label,
  active = false,
  color = "bg-red-600",
}: {
  label: string;
  active?: boolean;
  color?: string;
}) {
  return (
    <div className="flex items-center gap-2 mb-1">
      <div
        className={`w-3 h-3 border border-gray-600 ${active ? `${color} shadow-[0_0_8px_rgba(220,38,38,0.8)]` : "bg-red-950/40"}`}
      ></div>
      <span className="text-gray-200 text-[10px] font-bold font-sans tracking-wide leading-none">
        {label}
      </span>
    </div>
  );
}
