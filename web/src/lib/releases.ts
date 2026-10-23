import { useEffect, useState } from "react"

import { REPO } from "@/lib/links"

export type Releases = {
  /** Downloads of every installer across all releases. */
  downloads: number
  version: string
  mac?: string
  windows?: string
}

type GhRelease = {
  tag_name: string
  draft: boolean
  prerelease: boolean
  assets: { name: string; browser_download_url: string; download_count: number }[]
}

const isInstaller = (name: string) => /\.(pkg|exe)$/.test(name)

/** Latest installers and the total download count, from the GitHub API. Null until loaded or on failure. */
export function useReleases() {
  const [data, setData] = useState<Releases | null>(null)
  useEffect(() => {
    const ctrl = new AbortController()
    fetch(`https://api.github.com/repos/${REPO}/releases?per_page=100`, { signal: ctrl.signal })
      .then((r) => (r.ok ? r.json() : Promise.reject(r.status)))
      .then((all: GhRelease[]) => {
        const published = all.filter((r) => !r.draft && !r.prerelease)
        const latest = published[0]
        if (!latest) return
        const asset = (ext: string) => latest.assets.find((a) => a.name.endsWith(ext))?.browser_download_url
        setData({
          downloads: published.flatMap((r) => r.assets).filter((a) => isInstaller(a.name)).reduce((n, a) => n + a.download_count, 0),
          version: latest.tag_name.replace(/^v/, ""),
          mac: asset(".pkg"),
          windows: asset(".exe"),
        })
      })
      .catch(() => { /* offline or rate-limited: fall back to the releases page */ })
    return () => ctrl.abort()
  }, [])
  return data
}
