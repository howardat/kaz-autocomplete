import { useMemo } from "react"
import { ArrowRight, Check, X } from "lucide-react"

import { Kbd } from "@/components/ui/kbd"
import { convertText, type Engine } from "@/lib/engine"
import { useI18n } from "@/lib/i18n"

/** Words people commonly type with Russian letters in place of Kazakh ones. */
const TRICKY = [
  "жексенби куни", "агылшын тили", "калыныз калай", "откен жылы",
  "олен", "тауелсиз", "кыскаша", "бугин сагат",
]

function BenefitCard({ pain, title, text, children }: { pain: string; title: string; text: string; children: React.ReactNode }) {
  return (
    <div className="flex flex-col gap-4 rounded-xl border bg-card/60 p-5 backdrop-blur">
      <div className="grid min-h-28 place-items-center rounded-lg border border-dashed bg-muted/30 px-3 py-4">{children}</div>
      <div className="flex flex-col gap-2">
        <span className="flex items-center gap-1.5 text-xs text-muted-foreground">
          <X className="size-3.5 text-destructive" /> <span className="line-through decoration-muted-foreground/50">{pain}</span>
        </span>
        <h3 className="flex items-center gap-1.5 text-lg font-medium">
          <Check className="size-4 text-kaz" /> {title}
        </h3>
        <p className="text-sm text-muted-foreground">{text}</p>
      </div>
    </div>
  )
}

const Key = ({ children, kaz }: { children: React.ReactNode; kaz?: string }) => (
  <span className="relative grid size-11 place-items-center rounded-lg border border-kaz/40 bg-kaz/10 text-lg uppercase shadow-[0_2px_0_var(--border)]">
    {children}
    {kaz && <span className="absolute top-0.5 right-1 text-xs normal-case text-kaz">{kaz}</span>}
  </span>
)

/** What KazType fixes about typing Kazakh, and the words that trip people up. */
export function Problems({ engine, onTry }: { engine: Engine | null; onTry: (text: string) => void }) {
  const { t } = useI18n()
  const fixed = useMemo(() => (engine ? TRICKY.map((w) => convertText(engine, w)) : null), [engine])
  const [b1, b2, b3] = t.benefits

  return (
    <section id="why" className="mx-auto max-w-6xl scroll-mt-20 px-4 pb-24">
      <div className="mx-auto mb-10 max-w-2xl text-center">
        <h2 className="text-3xl font-semibold tracking-tight text-balance">{t.benefitsTitle}</h2>
        <p className="mt-3 text-muted-foreground">{t.benefitsText}</p>
      </div>

      <div className="grid gap-4 md:grid-cols-3">
        <BenefitCard {...b1}>
          <span className="flex items-center gap-3">
            <Key kaz="қ">к</Key>
            <ArrowRight className="size-4 text-muted-foreground" />
            <span className="text-3xl font-medium text-kaz">қ</span>
          </span>
        </BenefitCard>

        <BenefitCard {...b2}>
          <div className="flex flex-col items-center gap-2">
            <span className="flex items-center gap-1.5 text-muted-foreground line-through decoration-destructive">
              <Kbd className="h-7 px-2 text-sm">Alt</Kbd>+<Kbd className="h-7 px-2 text-sm">Shift</Kbd>
            </span>
            <span className="flex items-center gap-1.5 rounded-full border bg-background px-3 py-1 text-sm font-medium">
              <span className="size-1.5 rounded-full bg-kaz" /> RU · ЙЦУКЕН
            </span>
          </div>
        </BenefitCard>

        <BenefitCard {...b3}>
          <div className="flex flex-col items-center gap-2.5 text-lg">
            <span className="flex items-center gap-2">
              <span className="text-muted-foreground line-through decoration-destructive/70">жексенби куни</span>
              <ArrowRight className="size-4 text-muted-foreground" />
              <Diff typed="жексенби куни" fixed="жексенбі күні" />
            </span>
            <span className="flex flex-wrap items-center justify-center gap-1.5 text-sm">
              <span className="font-mono text-muted-foreground">кол</span>
              <ArrowRight className="size-3.5 text-muted-foreground" />
              <span className="flex items-center gap-1 rounded-md border bg-background px-1.5 py-0.5">қол <Kbd>1</Kbd></span>
              <span className="flex items-center gap-1 rounded-md border bg-background px-1.5 py-0.5">көл <Kbd>2</Kbd></span>
            </span>
          </div>
        </BenefitCard>
      </div>

      <div className="mt-10 rounded-xl border bg-card/60 p-5 backdrop-blur">
        <div className="mb-4 flex flex-wrap items-baseline justify-between gap-2">
          <h3 className="font-medium">{t.trickyTitle}</h3>
          <span className="text-sm text-muted-foreground">{t.trickyHint}</span>
        </div>
        <div className="grid gap-2 sm:grid-cols-2 lg:grid-cols-4">
          {TRICKY.map((typed, i) => (
            <button
              key={typed}
              type="button"
              onClick={() => onTry(typed + " ")}
              className="group flex flex-col items-start gap-0.5 rounded-lg border bg-background/60 px-3 py-2.5 text-left transition-colors hover:border-kaz/50 hover:bg-kaz/5"
            >
              <span className="font-mono text-xs text-muted-foreground">{typed}</span>
              <span className="flex w-full items-center justify-between gap-2 text-lg">
                {fixed ? <Diff typed={typed} fixed={fixed[i]} /> : <span className="text-muted-foreground">…</span>}
                <ArrowRight className="size-4 shrink-0 text-muted-foreground opacity-0 transition-all group-hover:translate-x-0.5 group-hover:opacity-100" />
              </span>
            </button>
          ))}
        </div>
      </div>
    </section>
  )
}

/** The converted text with the letters KazType changed highlighted. */
function Diff({ typed, fixed }: { typed: string; fixed: string }) {
  if (typed.length !== fixed.length) return <span>{fixed}</span>
  return (
    <span>
      {[...fixed].map((c, i) => (c === typed[i] ? c : <b key={i} className="font-medium text-kaz">{c}</b>))}
    </span>
  )
}
