import { useCallback, useEffect, useLayoutEffect, useRef, useState } from "react"
import { RotateCcw, Zap } from "lucide-react"

import { Button } from "@/components/ui/button"
import { Kbd } from "@/components/ui/kbd"
import type { Engine } from "@/lib/engine"
import { useI18n } from "@/lib/i18n"
import { emptyIme, step, type ImeAction, type ImeState } from "@/lib/ime"
import { russianKey } from "@/lib/keys"
import { cn } from "@/lib/utils"

const EXAMPLES = ["казакстан", "бугин ауа райы жаксы", "менин атым", "рахмет, сау болыныз"]

/** A text box that behaves like the KazType input method. */
export function TypePad({ engine }: { engine: Engine }) {
  const { t } = useI18n()
  const [s, setS] = useState<ImeState>(emptyIme)
  const [focused, setFocused] = useState(false)
  const input = useRef<HTMLTextAreaElement>(null)
  const word = useRef<HTMLSpanElement>(null)
  const box = useRef<HTMLDivElement>(null)
  const [popPos, setPopPos] = useState({ left: 0, top: 0 })

  const dispatch = useCallback((...actions: ImeAction[]) => {
    setS((prev) => actions.reduce((acc, a) => step(engine, acc, a), prev))
  }, [engine])

  const typeString = (str: string) => {
    dispatch({ type: "commit" }, ...[...str].map((char) => ({ type: "char", char }) as ImeAction), { type: "commit" })
    input.current?.focus()
  }

  // Phones send text through beforeinput instead of key events.
  useEffect(() => {
    const el = input.current
    if (!el) return
    const onBeforeInput = (e: InputEvent) => {
      e.preventDefault()
      if (e.inputType === "deleteContentBackward") dispatch({ type: "backspace" })
      else if (e.inputType === "insertLineBreak") dispatch({ type: "enter" })
      else if (e.data) dispatch(...[...e.data].map((char) => ({ type: "char", char }) as ImeAction))
    }
    el.addEventListener("beforeinput", onBeforeInput)
    return () => el.removeEventListener("beforeinput", onBeforeInput)
  }, [dispatch])

  const onKeyDown = (e: React.KeyboardEvent) => {
    if (e.key === "Unidentified" || e.key === "Process" || e.nativeEvent.isComposing) return
    if (e.metaKey || e.ctrlKey || e.altKey) {
      if (e.key.length === 1) dispatch({ type: "commit" })
      return
    }
    const actions: Record<string, ImeAction> = {
      Backspace: { type: "backspace" },
      Escape: { type: "escape" },
      Enter: { type: "enter" },
      ArrowDown: { type: "move", delta: 1 },
      ArrowUp: { type: "move", delta: -1 },
    }
    let action: ImeAction | undefined = actions[e.key]
    if (!action) {
      const c = russianKey(e)
      if (c) action = { type: "char", char: c }
    }
    if (action) {
      e.preventDefault()
      dispatch(action)
    }
  }

  useLayoutEffect(() => {
    if (!word.current || !box.current) return
    const w = word.current.getBoundingClientRect()
    const b = box.current.getBoundingClientRect()
    setPopPos({ left: w.left - b.left, top: w.bottom - b.top + 6 })
  }, [s.typed, s.result])

  const candidates = s.result?.candidates ?? []
  const shown = s.result ? candidates[s.selected] : ""
  const empty = !s.text && !s.typed

  return (
    <div className="flex flex-col gap-3">
      <div
        ref={box}
        onClick={() => input.current?.focus()}
        className={cn(
          "relative min-h-52 cursor-text rounded-xl border bg-background/60 p-5 text-xl leading-relaxed transition-shadow sm:text-2xl",
          focused ? "border-ring ring-[3px] ring-ring/30" : "hover:border-ring/50",
        )}
      >
        <textarea
          ref={input}
          aria-label={t.padLabel}
          className="absolute inset-0 h-full w-full resize-none opacity-0"
          autoCapitalize="off"
          autoComplete="off"
          autoCorrect="off"
          spellCheck={false}
          onKeyDown={onKeyDown}
          onFocus={() => setFocused(true)}
          onBlur={() => setFocused(false)}
          value=""
          onChange={() => {}}
        />
        <p className="pointer-events-none break-words whitespace-pre-wrap">
          {s.text}
          {s.typed && (
            <span ref={word} className="rounded-sm bg-primary/[0.06] underline decoration-foreground/60 decoration-2 underline-offset-[6px]">
              {shown}
            </span>
          )}
          {focused && <span className="ml-px inline-block h-[1.1em] w-0.5 translate-y-[0.15em] animate-pulse bg-foreground" />}
          {empty && !focused && (
            <span className="text-muted-foreground">{t.padPlaceholder}</span>
          )}
        </p>

        {candidates.length > 1 && (
          <div
            style={popPos}
            className="absolute z-10 min-w-48 animate-in rounded-lg border bg-popover p-1 text-base text-popover-foreground shadow-xl shadow-black/10 fade-in-0 zoom-in-95 duration-100"
          >
            {candidates.map((c, i) => (
              <button
                key={c + i}
                type="button"
                onMouseDown={(e) => e.preventDefault()}
                onMouseEnter={() => dispatch({ type: "hover", index: i })}
                onClick={() => dispatch({ type: "pick", index: i })}
                className={cn(
                  "flex w-full items-center justify-between gap-6 rounded-md px-2.5 py-1.5 text-left",
                  i === s.selected && "bg-accent text-accent-foreground",
                )}
              >
                <span>{c}</span>
                <Kbd className={cn(i === s.selected && "bg-background")}>{i + 1}</Kbd>
              </button>
            ))}
          </div>
        )}
      </div>

      <div className="flex flex-wrap items-center justify-between gap-3 text-sm text-muted-foreground">
        <div className="flex flex-wrap items-center gap-x-3 gap-y-1">
          <span className="flex items-center gap-1"><Kbd>↑</Kbd><Kbd>↓</Kbd> {t.keyChoose}</span>
          <span className="flex items-center gap-1"><Kbd>1</Kbd>–<Kbd>9</Kbd> {t.keyPick}</span>
          <span className="flex items-center gap-1"><Kbd>Space</Kbd> {t.keyCommit}</span>
          <span className="flex items-center gap-1"><Kbd>Esc</Kbd> {t.keyEscape}</span>
        </div>
        <div className="flex items-center gap-2">
          {s.ms !== null && (
            <span className="flex items-center gap-1 font-mono text-xs tabular-nums">
              <Zap className="size-3" /> {s.ms.toFixed(1)} {t.onDevice}
            </span>
          )}
          <Button variant="ghost" size="sm" onClick={() => dispatch({ type: "clear" })} disabled={empty}>
            <RotateCcw /> {t.clear}
          </Button>
        </div>
      </div>

      <div className="flex flex-wrap items-center gap-2 text-sm">
        <span className="text-muted-foreground">{t.try}</span>
        {EXAMPLES.map((ex) => (
          <Button key={ex} variant="outline" size="sm" className="rounded-full font-normal" onClick={() => typeString(ex + " ")}>
            {ex}
          </Button>
        ))}
      </div>
      <p className="text-xs text-muted-foreground">
        {t.noKeyboard}
      </p>
    </div>
  )
}
