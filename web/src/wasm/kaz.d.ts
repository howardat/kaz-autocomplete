// Types for the Emscripten module built by wasm/build.sh.
export interface KazModule {
  FS: { writeFile(path: string, data: Uint8Array): void }
  cwrap(name: string, ret: string | null, args: string[]): (...args: unknown[]) => unknown
}
export default function createModule(opts?: { locateFile?: (path: string) => string }): Promise<KazModule>
