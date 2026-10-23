import { cn } from "@/lib/utils"

const ROWS = ["йцукенгшщзхъ", "фывапролджэ", "ячсмитьбю"]

/** Kazakh letters KazType restores from each Russian key. */
const KAZAKH_ON: Record<string, string> = {
  а: "ә", г: "ғ", и: "і", к: "қ", н: "ң", о: "ө", у: "ұү", х: "һ",
}

/** The Russian ЙЦУКЕН layout, with the eight keys that hide Kazakh letters marked. */
export function Keyboard({ active, className }: { active: string | null; className?: string }) {
  return (
    <div aria-hidden className={cn("flex flex-col items-center gap-0.5 select-none sm:gap-1.5", className)}>
      {ROWS.map((row, r) => (
        <div key={row} className="flex gap-0.5 sm:gap-1.5" style={{ paddingLeft: `${r * 0.6}rem` }}>
          {[...row].map((key) => {
            const kaz = KAZAKH_ON[key]
            const on = active === key
            return (
              <span
                key={key}
                className={cn(
                  "relative grid h-8 w-5 place-items-center rounded-md border text-xs uppercase shadow-[0_1px_0_var(--border)] transition-all duration-100 sm:h-10 sm:w-9 sm:text-sm",
                  kaz ? "border-kaz/40 bg-kaz/10 text-foreground" : "bg-background/60 text-muted-foreground",
                  on && "translate-y-px scale-95 border-foreground bg-foreground text-background shadow-none",
                )}
              >
                {key}
                {kaz && (
                  <span className={cn("absolute top-0 right-0.5 text-[0.55rem] leading-tight normal-case sm:text-[0.65rem]", on ? "text-background" : "text-kaz")}>
                    {kaz}
                  </span>
                )}
              </span>
            )
          })}
        </div>
      ))}
    </div>
  )
}
