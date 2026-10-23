import { useCallback, useEffect, useImperativeHandle, useLayoutEffect, useRef, useState } from "react"
import { RotateCcw, Zap } from "lucide-react"

import { Keyboard } from "@/components/keyboard"
import { Button } from "@/components/ui/button"
import { Kbd } from "@/components/ui/kbd"
import type { Engine } from "@/lib/engine"
import { useI18n } from "@/lib/i18n"
import { emptyIme, step, type ImeAction, type ImeState } from "@/lib/ime"
import { isCyrillicLetter, russianKey } from "@/lib/keys"
import { cn } from "@/lib/utils"

const EXAMPLES = ["жексенби куни", "агылшын тили", "калыныз калай", "бул ул"]

/** A placeholder that types itself, holds, erases and types again, inviting a click. */
function TypedPlaceholder({ text }: { text: string }) {
  const still = window.matchMedia("(prefers-reduced-motion: reduce)").matches
  const [n, setN] = useState(still ? text.length : 0)
  const [erasing, setErasing] = useState(false)

  useEffect(() => {
    if (still) return
    let delay: number
    let next: () => void
    if (erasing) {
      delay = n > 0 ? 25 : 500
      next = n > 0 ? () => setN(n - 1) : () => setErasing(false)
    } else if (n < text.length) {
      delay = 55 + (n % 4) * 20
      next = () => setN(n + 1)
    } else {
      delay = 2400
      next = () => setErasing(true)
    }
    const id = window.setTimeout(next, delay)
    return () => window.clearTimeout(id)
  }, [n, erasing, text, still])

  return (
    <span className="text-muted-foreground">
      {text.slice(0, n)}
      <span className="ml-px inline-block h-[1.1em] w-0.5 translate-y-[0.15em] animate-pulse bg-kaz" />
    </span>
  )
}

export type TypePadHandle = {
  /** Types `text` as if the user had. */
  type(text: string): void
}

/** A text box that behaves like the KazType input method. */
export function TypePad({ engine, ref }: { engine: Engine; ref?: React.Ref<TypePadHandle> }) {
  const { t } = useI18n()
  const [s, setS] = useState<ImeState>(emptyIme)
  const [focused, setFocused] = useState(false)
  const [pressed, setPressed] = useState<string | null>(null)
  const input = useRef<HTMLTextAreaElement>(null)
  const word = useRef<HTMLSpanElement>(null)
  const box = useRef<HTMLDivElement>(null)
  const pressTimer = useRef(0)
  const [popPos, setPopPos] = useState({ left: 0, top: 0 })

  const dispatch = useCallback((...actions: ImeAction[]) => {
    setS((prev) => actions.reduce((acc, a) => step(engine, acc, a), prev))
  }, [engine])

  const flash = useCallback((char: string) => {
    if (!isCyrillicLetter(char)) return
    setPressed(char.toLowerCase())
    clearTimeout(pressTimer.current)
    pressTimer.current = window.setTimeout(() => setPressed(null), 140)
  }, [])

  const typeString = (str: string) => {
    dispatch({ type: "commit" }, ...[...str].map((char) => ({ type: "char", char }) as ImeAction), { type: "commit" })
    input.current?.focus({ preventScroll: true })
  }

  useImperativeHandle(ref, () => ({
    type(text) {
      box.current?.scrollIntoView({ behavior: "smooth", block: "center" })
      typeString(text)
    },
  }))

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
      if (c) {
        action = { type: "char", char: c }
        flash(c)
      }
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
    <div className="flex flex-col gap-4">
      <div
        ref={box}
        onClick={() => input.current?.focus()}
        className={cn(
          "relative min-h-52 cursor-text rounded-xl border-2 bg-background p-5 text-xl leading-relaxed transition-shadow sm:text-2xl",
          focused ? "border-kaz ring-4 ring-kaz/20" : "border-dashed border-kaz/50 hover:border-kaz",
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
            <span ref={word} className="rounded-sm bg-kaz/10 underline decoration-kaz decoration-2 underline-offset-[6px]">
              {shown}
            </span>
          )}
          {focused && <span className="ml-px inline-block h-[1.1em] w-0.5 translate-y-[0.15em] animate-pulse bg-foreground" />}
          {empty && focused && <span className="text-muted-foreground">{t.padPlaceholder}</span>}
          {empty && !focused && <TypedPlaceholder key={t.padCta} text={t.padCta} />}
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
                tabIndex={-1}
                onMouseDown={(e) => e.preventDefault()}
                onMouseEnter={() => dispatch({ type: "hover", index: i })}
                onClick={() => dispatch({ type: "pick", index: i })}
                className={cn(
                  "flex w-full items-center justify-between gap-6 rounded-md px-2.5 py-1.5 text-left transition-colors",
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

      <div className="flex min-h-9 flex-wrap items-center justify-between gap-3 text-sm text-muted-foreground">
        <div className="flex flex-wrap items-center gap-x-3 gap-y-1">
          <span className="flex items-center gap-1"><Kbd>↑</Kbd><Kbd>↓</Kbd> {t.keyChoose}</span>
          <span className="flex items-center gap-1"><Kbd>1</Kbd>–<Kbd>9</Kbd> {t.keyPick}</span>
          <span className="flex items-center gap-1"><Kbd>Space</Kbd> {t.keyCommit}</span>
          <span className="flex items-center gap-1"><Kbd>Esc</Kbd> {t.keyEscape}</span>
        </div>
        <div className="flex items-center gap-1">
          {s.ms !== null && (
            <span className="mr-1 flex items-center gap-1 font-mono text-xs tabular-nums">
              <Zap className="size-3" /> {s.ms.toFixed(1)} {t.onDevice}
            </span>
          )}
          <Button variant="ghost" size="sm" onClick={() => dispatch({ type: "clear" })} disabled={empty}>
            <RotateCcw /> {t.clear}
          </Button>
        </div>
      </div>

      <div className="flex flex-col items-center gap-3 rounded-xl border border-dashed bg-muted/30 px-3 py-4">
        <Keyboard active={pressed} />
        <p className="max-w-md text-center text-xs text-muted-foreground">{t.keyboardHint}</p>
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
