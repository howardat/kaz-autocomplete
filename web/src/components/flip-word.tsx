import { useEffect, useState } from "react"

import { cn } from "@/lib/utils"

const TYPE_MS = 110
const FLIP_GAP_MS = 220
const HOLD_MS = 2600
const ERASE_MS = 35

type Phase = { shown: number; flipped: number; erasing: boolean; started: boolean }

/**
 * `typed` in Russian letters, then each letter KazType changes flips into its Kazakh one
 * from `fixed` (the same flip as in the hero video). Holds, erases, repeats.
 * `typed` and `fixed` line up letter for letter.
 */
export function FlipWord({
  typed: TYPED,
  fixed: FIXED,
  startMs = 0,
  holdMs = HOLD_MS,
  className,
}: {
  typed: string
  fixed: string
  startMs?: number
  holdMs?: number
  className?: string
}) {
  const changed = [...TYPED].flatMap((c, i) => (c === FIXED[i] ? [] : [i]))
  const still = typeof window !== "undefined" && window.matchMedia("(prefers-reduced-motion: reduce)").matches
  const [p, setP] = useState<Phase>(() =>
    still
      ? { shown: TYPED.length, flipped: changed.length, erasing: false, started: true }
      : { shown: 0, flipped: 0, erasing: false, started: false },
  )

  useEffect(() => {
    if (still) return
    let delay: number
    let next: Phase
    if (!p.started) {
      next = { ...p, started: true }
      delay = startMs
    } else if (p.erasing) {
      next = p.shown > 0 ? { ...p, shown: p.shown - 1 } : { shown: 0, flipped: 0, erasing: false, started: true }
      delay = p.shown > 0 ? ERASE_MS : 500
    } else if (p.shown < TYPED.length) {
      next = { ...p, shown: p.shown + 1 }
      delay = TYPE_MS + (p.shown % 3) * 25
    } else if (p.flipped < changed.length) {
      next = { ...p, flipped: p.flipped + 1 }
      delay = p.flipped === 0 ? 350 : FLIP_GAP_MS
    } else {
      next = { ...p, erasing: true }
      delay = holdMs
    }
    const id = window.setTimeout(() => setP(next), delay)
    return () => window.clearTimeout(id)
  }, [p, still, changed.length, TYPED.length, startMs, holdMs])

  const typing = !still && p.shown < TYPED.length

  return (
    <span className={cn("relative inline-grid text-left align-baseline text-foreground", className)} aria-label={FIXED}>
      {/* Holds the width of the typed phrase, the wider one, so the sentence around it never moves. */}
      <span className="invisible col-start-1 row-start-1 whitespace-pre" aria-hidden>
        {TYPED}
        <span className="ml-px inline-block w-[max(2px,0.06em)]" />
      </span>
      <span className="col-start-1 row-start-1 whitespace-pre" aria-hidden>
        {[...TYPED.slice(0, p.shown)].map((c, i) => {
          const at = changed.indexOf(i)
          if (at < 0 || at >= p.flipped) return <span key={i}>{c}</span>
          return (
            <span key={i} className="relative inline-block text-kaz [perspective:400px]">
              <span className="kz-flip-out absolute top-0 left-0">{c}</span>
              <span className="kz-flip-in inline-block">{FIXED[i]}</span>
            </span>
          )
        })}
        <span
          className={cn(
            "ml-px inline-block h-[0.8em] w-[max(2px,0.06em)] align-[-0.08em] bg-foreground",
            typing || p.erasing ? "" : "animate-pulse",
          )}
        />
      </span>
    </span>
  )
}
