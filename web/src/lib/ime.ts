import type { Engine, Result } from "@/lib/engine"
import { isCyrillicLetter } from "@/lib/keys"

/** The input method's state machine, same rules as the macOS and Windows apps. */
export type ImeState = {
  /** Committed text. */
  text: string
  /** Word being typed, as typed. */
  typed: string
  /** Text before the word, read when it started. */
  context: string
  result: Result | null
  selected: number
  /** Last conversion time in ms. */
  ms: number | null
}

export const emptyIme: ImeState = { text: "", typed: "", context: "", result: null, selected: 0, ms: null }

export type ImeAction =
  | { type: "char"; char: string }
  | { type: "backspace" }
  | { type: "escape" }
  | { type: "enter" }
  | { type: "move"; delta: number }
  | { type: "pick"; index: number }
  | { type: "hover"; index: number }
  | { type: "commit" }
  | { type: "clear" }

function convert(engine: Engine, s: ImeState, typed: string, context: string): ImeState {
  const t = performance.now()
  const result = engine.convert(typed, context, 9)
  return { ...s, typed, context, result, selected: 0, ms: performance.now() - t }
}

function commit(engine: Engine, s: ImeState): ImeState {
  if (!s.typed || !s.result) return s
  const chosen = s.result.candidates[s.selected]
  engine.learn(s.typed, chosen, s.result.modelTop)
  return { ...s, text: s.text + chosen, typed: "", context: "", result: null, selected: 0 }
}

export function step(engine: Engine, s: ImeState, a: ImeAction): ImeState {
  const n = s.result?.candidates.length ?? 0
  switch (a.type) {
    case "char":
      if (isCyrillicLetter(a.char)) {
        return convert(engine, s, s.typed + a.char, s.typed ? s.context : s.text.slice(-64))
      }
      // Digits pick a candidate while the list is open.
      if (s.typed && /^[1-9]$/.test(a.char) && n > 1 && +a.char <= n) {
        return commit(engine, { ...s, selected: +a.char - 1 })
      }
      // Space, punctuation, anything else: commit, then type the key.
      return (s2 => ({ ...s2, text: s2.text + a.char }))(commit(engine, s))
    case "backspace":
      if (!s.typed) return { ...s, text: s.text.slice(0, -1) }
      if (s.typed.length === 1) return { ...s, typed: "", context: "", result: null, selected: 0 }
      return convert(engine, s, s.typed.slice(0, -1), s.context)
    case "escape":
      return { ...s, text: s.text + s.typed, typed: "", context: "", result: null, selected: 0 }
    case "enter":
      return s.typed ? commit(engine, s) : { ...s, text: s.text + "\n" }
    case "move":
      return n > 1 ? { ...s, selected: (s.selected + a.delta + n) % n } : s
    case "hover":
      return a.index < n ? { ...s, selected: a.index } : s
    case "pick":
      return a.index < n ? commit(engine, { ...s, selected: a.index }) : s
    case "commit":
      return commit(engine, s)
    case "clear":
      return { ...emptyIme, ms: s.ms }
  }
}
