import { useI18n } from "@/lib/i18n"

/**
 * The hero video (videos/kaztype-hero): one sentence typed across apps, its "works in" dock drawn
 * into it. One render per interface language, since the dock's caption is part of the picture.
 */
export function Showcase() {
  const { lang, t } = useI18n()
  return (
    <video
      key={lang}
      className="block aspect-video w-full rounded-2xl border bg-muted shadow-2xl shadow-black/10"
      autoPlay
      muted
      loop
      playsInline
      preload="auto"
      poster={`/hero/hero-${lang}-poster.jpg`}
      aria-label={t.videoLabel}
    >
      <source src={`/hero/hero-${lang}.mp4`} type="video/mp4" />
      <source src={`/hero/hero-${lang}.webm`} type="video/webm" />
    </video>
  )
}
