/** Russian ЙЦУКЕН letters by physical key, so any keyboard can type in the demo. */
const LETTERS: Record<string, string> = {
  KeyQ: "й", KeyW: "ц", KeyE: "у", KeyR: "к", KeyT: "е", KeyY: "н", KeyU: "г", KeyI: "ш",
  KeyO: "щ", KeyP: "з", BracketLeft: "х", BracketRight: "ъ",
  KeyA: "ф", KeyS: "ы", KeyD: "в", KeyF: "а", KeyG: "п", KeyH: "р", KeyJ: "о", KeyK: "л",
  KeyL: "д", Semicolon: "ж", Quote: "э",
  KeyZ: "я", KeyX: "ч", KeyC: "с", KeyV: "м", KeyB: "и", KeyN: "т", KeyM: "ь", Comma: "б",
  Period: "ю", Backquote: "ё",
}

/** Shifted symbols that differ on the Russian layout. */
const SHIFTED: Record<string, string> = {
  Digit2: '"', Digit3: "№", Digit4: ";", Digit6: ":", Digit7: "?", Slash: ",", Backslash: "/",
}

export const isCyrillicLetter = (c: string) => /^[Ѐ-ӿ]$/.test(c) && c.toLowerCase() !== c.toUpperCase()

/** What a key types on a Russian keyboard, or null for non-character keys. */
export function russianKey(e: KeyboardEvent | React.KeyboardEvent): string | null {
  if (e.key.length === 1 && isCyrillicLetter(e.key)) return e.key // already a Russian layout
  const letter = LETTERS[e.code]
  if (letter) {
    const upper = e.shiftKey !== e.getModifierState("CapsLock")
    return upper ? letter.toUpperCase() : letter
  }
  if (e.shiftKey && SHIFTED[e.code]) return SHIFTED[e.code]
  if (!e.shiftKey && e.code === "Slash") return "."
  return e.key.length === 1 ? e.key : null
}
