import createModule from "@/wasm/kaz.js"
import wasmUrl from "@/wasm/kaz.wasm?url"

export type Result = {
  /** Best first. The typed word is always included. */
  candidates: string[]
  /** What the model alone would pick, before the user's earlier choices. */
  modelTop: string
}

export type Engine = {
  convert(typed: string, context?: string, limit?: number): Result
  learn(typed: string, chosen: string, modelTop: string): void
}

/** Loads the same C++ engine and model the Windows input method uses. */
export async function loadEngine(): Promise<Engine> {
  const [mod, bin] = await Promise.all([
    createModule({ locateFile: () => wasmUrl }),
    fetch("/kaztype.bin").then((r) => {
      if (!r.ok) throw new Error(`model download failed: ${r.status}`)
      return r.arrayBuffer()
    }),
  ])
  mod.FS.writeFile("/kaztype.bin", new Uint8Array(bin))
  if (!mod.cwrap("kaz_load", "number", ["string"])("/kaztype.bin")) throw new Error("model failed to load")

  const convert = mod.cwrap("kaz_convert", "string", ["string", "string", "number"])
  const learn = mod.cwrap("kaz_learn", null, ["string", "string", "string"])
  return {
    convert(typed, context = "", limit = 9) {
      const lines = (convert(typed, context, limit) as string).split("\n")
      return { candidates: lines.slice(0, -1), modelTop: lines[lines.length - 1] }
    },
    learn: (typed, chosen, modelTop) => void learn(typed, chosen, modelTop),
  }
}

const CYRILLIC_WORD = /[Ѐ-ӿ]+/g

/** Converts a whole text, word by word, each with the text before it as context. */
export function convertText(engine: Engine, text: string): string {
  let out = ""
  let last = 0
  for (const m of text.matchAll(CYRILLIC_WORD)) {
    out += text.slice(last, m.index)
    out += engine.convert(m[0], out.slice(-64), 1).candidates[0]
    last = m.index + m[0].length
  }
  return out + text.slice(last)
}
