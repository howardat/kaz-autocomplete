import { useDeferredValue, useEffect, useMemo, useRef, useState } from "react"
import { ArrowRight, Check, Copy, Download, Heart, Keyboard, Loader2, Moon, Share2, Sun } from "lucide-react"

import { AppleIcon, GitHubIcon, InstagramIcon, LinkedInIcon, WindowsIcon } from "@/components/brand-icons"
import { Problems } from "@/components/problems"
import { Showcase } from "@/components/showcase"
import { TypePad, type TypePadHandle } from "@/components/type-pad"
import { Badge } from "@/components/ui/badge"
import { Button } from "@/components/ui/button"
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from "@/components/ui/card"
import { Separator } from "@/components/ui/separator"
import { Tabs, TabsContent, TabsList, TabsTrigger } from "@/components/ui/tabs"
import { Textarea } from "@/components/ui/textarea"
import { convertText, loadEngine, type Engine } from "@/lib/engine"
import { LANGS, useI18n } from "@/lib/i18n"
import { AUTHOR, KASPI_PHONE, LICENSE_URL, RELEASES_URL, REPO_URL, SOCIALS } from "@/lib/links"
import { useReleases } from "@/lib/releases"
import { cn } from "@/lib/utils"

function useTheme() {
  const [dark, setDark] = useState(() => {
    try {
      const saved = localStorage.getItem("theme")
      if (saved) return saved === "dark"
    } catch { /* storage blocked */ }
    return window.matchMedia("(prefers-color-scheme: dark)").matches
  })
  useEffect(() => {
    document.documentElement.classList.toggle("dark", dark)
    try { localStorage.setItem("theme", dark ? "dark" : "light") } catch { /* storage blocked */ }
  }, [dark])
  return [dark, () => setDark((d) => !d)] as const
}

function Logo({ className = "size-7" }: { className?: string }) {
  return (
    <span className={`${className} inline-grid place-items-center rounded-lg bg-primary font-semibold text-primary-foreground`}>
      Қ
    </span>
  )
}

function CopyButton({ text }: { text: string }) {
  const { t } = useI18n()
  const [done, setDone] = useState(false)
  return (
    <Button
      variant="ghost"
      size="sm"
      disabled={!text}
      onClick={async () => {
        await navigator.clipboard.writeText(text)
        setDone(true)
        setTimeout(() => setDone(false), 1500)
      }}
    >
      {done ? <Check /> : <Copy />} {done ? t.copied : t.copy}
    </Button>
  )
}

function ConvertPad({ engine }: { engine: Engine }) {
  const { t } = useI18n()
  const [text, setText] = useState("Казакстан Республикасы - тауелсиз мемлекет. Бугин ауа райы оте жаксы.")
  const deferred = useDeferredValue(text)
  const out = useMemo(() => convertText(engine, deferred), [engine, deferred])
  return (
    <div className="grid gap-4 md:grid-cols-2">
      <div className="flex flex-col gap-2">
        <span className="text-sm font-medium text-muted-foreground">{t.convertFrom}</span>
        <Textarea value={text} onChange={(e) => setText(e.target.value)} className="min-h-52 resize-none text-lg" placeholder={t.convertPlaceholder} />
      </div>
      <div className="flex flex-col gap-2">
        <div className="flex items-center justify-between">
          <span className="text-sm font-medium text-muted-foreground">{t.convertTo}</span>
          <CopyButton text={out} />
        </div>
        <div className="min-h-52 rounded-md border bg-muted/40 px-3 py-2 text-lg break-words whitespace-pre-wrap">{out}</div>
      </div>
    </div>
  )
}

function Step({ n, children }: { n: number; children: React.ReactNode }) {
  return (
    <li className="flex gap-3">
      <span className="grid size-6 shrink-0 place-items-center rounded-full border bg-background font-mono text-xs">{n}</span>
      <span className="pt-0.5 text-sm text-muted-foreground">{children}</span>
    </li>
  )
}

function ShareButton() {
  const { t } = useI18n()
  const [copied, setCopied] = useState(false)
  const share = async () => {
    const url = window.location.origin + window.location.pathname
    if (navigator.share) {
      try { await navigator.share({ title: "KazType", text: t.shareText, url }) } catch { /* dismissed */ }
      return
    }
    await navigator.clipboard.writeText(url)
    setCopied(true)
    setTimeout(() => setCopied(false), 1500)
  }
  return (
    <Button variant="ghost" size="sm" onClick={share} aria-label={t.share}>
      {copied ? <Check /> : <Share2 />} <span className="hidden sm:inline">{copied ? t.linkCopied : t.share}</span>
    </Button>
  )
}

function DonateButton() {
  const { t } = useI18n()
  const [copied, setCopied] = useState(false)
  return (
    <Button
      variant="outline"
      size="sm"
      className="ml-2 h-auto py-1.5"
      title={`${t.donateKaspi}: ${KASPI_PHONE}`}
      onClick={async () => {
        await navigator.clipboard.writeText(KASPI_PHONE.replace(/\s/g, ""))
        setCopied(true)
        setTimeout(() => setCopied(false), 1500)
      }}
    >
      {copied ? <Check /> : <Heart className="text-rose-500" />}
      <span className="flex flex-col items-start leading-tight">
        <span>{copied ? t.numberCopied : t.donate}</span>
        <span className="font-mono text-xs text-muted-foreground">{t.donateKaspi} · {KASPI_PHONE}</span>
      </span>
    </Button>
  )
}

const SOCIAL_LINKS = [
  { href: SOCIALS.github, label: "GitHub", Icon: GitHubIcon },
  { href: SOCIALS.linkedin, label: "LinkedIn", Icon: LinkedInIcon },
  { href: SOCIALS.instagram, label: "Instagram", Icon: InstagramIcon },
].filter((s) => s.href)

function LanguageSwitcher() {
  const { lang, t, setLang } = useI18n()
  return (
    <div role="group" aria-label={t.language} className="flex items-center rounded-md border bg-background/60 p-0.5">
      {LANGS.map(({ code, label }) => (
        <button
          key={code}
          type="button"
          lang={code}
          aria-pressed={lang === code}
          onClick={() => setLang(code)}
          className={cn(
            "rounded-sm px-2 py-1 text-xs font-medium transition-colors",
            lang === code ? "bg-accent text-accent-foreground" : "text-muted-foreground hover:text-foreground",
          )}
        >
          {label}
        </button>
      ))}
    </div>
  )
}

export default function App() {
  const { t } = useI18n()
  const releases = useReleases()
  const [dark, toggleTheme] = useTheme()
  const [engine, setEngine] = useState<Engine | null>(null)
  const [error, setError] = useState<string | null>(null)
  const pad = useRef<TypePadHandle>(null)

  useEffect(() => {
    loadEngine().then(setEngine, (e: Error) => setError(e.message))
  }, [])

  return (
    <div className="relative min-h-svh overflow-x-clip bg-background text-foreground">
      {/* Backdrop: fine grid fading out, plus a soft glow behind the hero. */}
      <div aria-hidden className="pointer-events-none absolute inset-0 -z-0 [background-image:linear-gradient(to_right,var(--border)_1px,transparent_1px),linear-gradient(to_bottom,var(--border)_1px,transparent_1px)] [background-size:56px_56px] [mask-image:radial-gradient(ellipse_70%_50%_at_50%_0%,black,transparent)] opacity-60" />
      <div aria-hidden className="pointer-events-none absolute top-[-12rem] left-1/2 -z-0 h-[32rem] w-[60rem] -translate-x-1/2 rounded-full bg-[radial-gradient(closest-side,oklch(0.7_0.12_250/0.18),transparent)] blur-2xl" />

      <header className="sticky top-0 z-20 border-b border-transparent bg-background/70 backdrop-blur-lg supports-[backdrop-filter]:bg-background/50">
        <div className="mx-auto flex h-14 max-w-6xl items-center justify-between px-4">
          <a href="#" className="flex items-center gap-2 font-semibold tracking-tight">
            <Logo /> KazType
          </a>
          <nav className="flex items-center gap-1">
            <Button variant="ghost" size="sm" asChild className="hidden sm:inline-flex"><a href="#demo">{t.nav.demo}</a></Button>
            <Button variant="ghost" size="sm" asChild className="hidden sm:inline-flex"><a href="#why">{t.nav.why}</a></Button>
            <Button variant="ghost" size="sm" asChild><a href="#download">{t.nav.download}</a></Button>
            <ShareButton />
            <Button variant="ghost" size="icon" asChild className="hidden sm:inline-flex">
              <a href={REPO_URL} target="_blank" rel="noreferrer" aria-label="GitHub"><GitHubIcon /></a>
            </Button>
            <LanguageSwitcher />
            <Button variant="ghost" size="icon" aria-label={t.nav.theme} onClick={toggleTheme}>
              {dark ? <Sun /> : <Moon />}
            </Button>
          </nav>
        </div>
      </header>

      <main className="relative z-10">
        <section className="mx-auto flex max-w-4xl flex-col items-center px-4 pt-20 pb-12 text-center sm:pt-28">
          <div className="mb-6 flex flex-wrap justify-center gap-2">
            <Badge variant="outline" className="gap-2 rounded-full bg-background/60 px-3 py-1 backdrop-blur" asChild>
              <a href="#download">
                <span className="flex items-center gap-1"><AppleIcon className="size-3.5" /> macOS</span>
                <span className="text-muted-foreground">·</span>
                <span className="flex items-center gap-1"><WindowsIcon className="size-3" /> Windows</span>
              </a>
            </Badge>
            <Badge variant="outline" className="rounded-full bg-background/60 px-3 py-1 backdrop-blur" asChild>
              <a href={REPO_URL} target="_blank" rel="noreferrer"><GitHubIcon /> {t.openSource}</a>
            </Badge>
          </div>
          <h1 className="text-4xl font-semibold tracking-tighter text-balance sm:text-6xl">
            {t.heroLead}
            <span className="bg-gradient-to-b from-foreground to-foreground/50 bg-clip-text text-transparent">{t.heroAccent}</span>
          </h1>
          <p className="mt-5 max-w-2xl text-lg text-pretty text-muted-foreground">
            {t.heroText}
          </p>
          <div className="mt-8 flex flex-wrap justify-center gap-3">
            <Button size="lg" asChild><a href="#demo">{t.tryNow} <ArrowRight /></a></Button>
            <Button size="lg" variant="outline" asChild><a href="#download"><Download /> {t.download}</a></Button>
          </div>
          {releases && releases.downloads > 0 && (
            <p className="mt-4 text-sm text-muted-foreground tabular-nums">{t.downloads(releases.downloads)}</p>
          )}
        </section>

        <section className="mx-auto max-w-5xl px-4 pb-24">
          <Showcase />
        </section>

        <section id="demo" className="mx-auto max-w-4xl scroll-mt-20 px-4 pb-24">
          <div className="mx-auto mb-8 max-w-2xl text-center">
            <h2 className="text-3xl font-semibold tracking-tight">{t.tryTitle}</h2>
            <p className="mt-3 text-muted-foreground">{t.tryText}</p>
          </div>
          <Card className="overflow-visible border-border/60 bg-card/70 shadow-2xl shadow-black/5 backdrop-blur-xl">
            <CardContent>
              {error ? (
                <p className="py-16 text-center text-sm text-destructive">{t.loadError} {error}</p>
              ) : !engine ? (
                <div className="flex items-center justify-center gap-2 py-24 text-sm text-muted-foreground">
                  <Loader2 className="size-4 animate-spin" /> {t.loading}
                </div>
              ) : (
                <Tabs defaultValue="type">
                  <TabsList className="mb-4">
                    <TabsTrigger value="type"><Keyboard /> {t.tabType}</TabsTrigger>
                    <TabsTrigger value="convert"><Copy /> {t.tabConvert}</TabsTrigger>
                  </TabsList>
                  <TabsContent value="type"><TypePad engine={engine} ref={pad} /></TabsContent>
                  <TabsContent value="convert"><ConvertPad engine={engine} /></TabsContent>
                </Tabs>
              )}
            </CardContent>
          </Card>
        </section>

        <Problems engine={engine} onTry={(text) => pad.current?.type(text)} />

        <section id="download" className="mx-auto max-w-6xl scroll-mt-20 px-4 pb-24">
          <div className="mx-auto mb-10 max-w-2xl text-center">
            <h2 className="text-3xl font-semibold tracking-tight">{t.getTitle}</h2>
            <p className="mt-3 text-muted-foreground">
              {t.getText}
            </p>
          </div>
          <div className="grid gap-4 md:grid-cols-2">
            <Card className="bg-card/60 backdrop-blur">
              <CardHeader>
                <div className="flex items-center gap-2">
                  <WindowsIcon className="size-5" />
                  <CardTitle className="text-xl">{t.winTitle}</CardTitle>
                </div>
                <CardDescription>{t.winDesc}</CardDescription>
              </CardHeader>
              <CardContent className="flex flex-col gap-4">
                <Button asChild className="self-start">
                  <a href={releases?.windows ?? RELEASES_URL}><Download /> {t.downloadFile(".exe")}</a>
                </Button>
                <ol className="flex flex-col gap-3">
                  <Step n={1}>{t.winStep1}</Step>
                  <Step n={2}>{t.winStep2}</Step>
                </ol>
              </CardContent>
            </Card>
            <Card className="bg-card/60 backdrop-blur">
              <CardHeader>
                <div className="flex items-center gap-2">
                  <AppleIcon className="size-5" />
                  <CardTitle className="text-xl">macOS 13+</CardTitle>
                </div>
                <CardDescription>{t.macDesc}</CardDescription>
              </CardHeader>
              <CardContent className="flex flex-col gap-4">
                <Button asChild className="self-start">
                  <a href={releases?.mac ?? RELEASES_URL}><Download /> {t.downloadFile(".pkg")}</a>
                </Button>
                <ol className="flex flex-col gap-3">
                  <Step n={1}>{t.macStep1}</Step>
                  <Step n={2}>{t.macStep2}</Step>
                </ol>
              </CardContent>
            </Card>
          </div>
          <p className="mt-4 text-center text-sm text-muted-foreground">
            {releases && <>{t.version(releases.version)} · </>}
            <a href={`${REPO_URL}#build-and-install`} target="_blank" rel="noreferrer" className="underline underline-offset-4 hover:text-foreground">
              {t.buildFromSource}
            </a>
          </p>
        </section>
      </main>

      <Separator />
      <footer className="relative z-10 mx-auto flex max-w-6xl flex-col gap-6 px-4 py-8 text-sm text-muted-foreground">
        <div className="flex flex-col items-center justify-between gap-4 sm:flex-row">
          <div className="flex flex-col items-center gap-1 sm:items-start">
            <span className="flex items-center gap-2 font-medium text-foreground"><Logo className="size-5 text-xs" /> KazType</span>
            <span>{t.madeBy} <span className="text-foreground">{AUTHOR}</span>, {t.authorRole}</span>
          </div>
          <div className="flex items-center gap-1">
            {SOCIAL_LINKS.map(({ href, label, Icon }) => (
              <Button key={label} variant="ghost" size="icon" asChild>
                <a href={href} target="_blank" rel="noreferrer" aria-label={label}><Icon /></a>
              </Button>
            ))}
            <DonateButton />
          </div>
        </div>
        <div className="flex flex-col items-center justify-between gap-2 text-xs sm:flex-row">
          <span>{t.footer}</span>
          <a href={LICENSE_URL} target="_blank" rel="noreferrer" className="hover:text-foreground">{t.license}</a>
        </div>
      </footer>
    </div>
  )
}
