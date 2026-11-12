export function StatusLight({
  num,
  active,
}: {
  num: string;
  active?: boolean;
}) {
  return (
    <div className="flex flex-col items-center justify-center w-4">
      <span
        className={`text-[10px] ${active ? "text-gray-100" : "text-gray-600"}`}
      >
        {num}
      </span>
    </div>
  );
}
