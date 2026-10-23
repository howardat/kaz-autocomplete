import { createContext, useContext, useEffect, useState, type ReactNode } from "react"

import { FlipWord } from "@/components/flip-word"

export const LANGS = [
  { code: "kk", label: "Қаз" },
  { code: "ru", label: "Рус" },
  { code: "en", label: "Eng" },
] as const

export type Lang = (typeof LANGS)[number]["code"]

const DEFAULT_LANG: Lang = "kk"

const mono = (s: string) => <span className="font-mono">{s}</span>

const en = {
  metaDescription: "Type Kazakh on a regular Russian keyboard. KazType puts in ә, ғ, қ, ң, ө, ұ, ү, һ, і as you type. For Windows and macOS.",
  language: "Language",
  nav: { demo: "Demo", why: "Benefits", download: "Download", theme: "Toggle theme" },
  heroLead: "Type the way you're used to. ",
  heroAccent: <>Get proper <FlipWord typed="казакша" fixed="қазақша" holdMs={4500} />.</>,
  heroText: (
    <>
      Type <FlipWord typed="жексенби куни" fixed="жексенбі күні" startMs={1600} className="font-medium" /> on a regular Russian keyboard and the Kazakh letters go in by themselves, right as you type.
    </>
  ),
  tryNow: "Try it now",
  download: "Download",
  loadError: "Could not load the model:",
  loading: "Loading the model (2.6 MB)…",
  tabType: "Type",
  tabConvert: "Fix existing text",
  getTitle: "Get KazType",
  getText: (
    <>
      Works wherever you type: browsers, Word, Telegram, WhatsApp. Pick a word option with {mono("1–9")} and KazType
      remembers your choice.
    </>
  ),
  winTitle: "Windows 10 & 11",
  winDesc: "For any Windows PC: x64, x86 and ARM64.",
  winStep1: "Run the installer. It is not code-signed yet, so if SmartScreen warns, choose More info > Run anyway.",
  winStep2: <>Press {mono("Win + Space")} and choose “Kazakh – KazType”.</>,
  macDesc: "For Macs with Apple silicon and Intel.",
  macStep1: "Open the installer. It is not notarized yet, so if macOS blocks it, go to System Settings > Privacy & Security and click Open Anyway.",
  macStep2: "Choose “Қазақша (KazType)” in the input menu. If it is missing, log out and back in.",
  downloadFile: (ext: string) => `Download ${ext}`,
  version: (v: string) => `Version ${v}`,
  downloads: (n: number) => `${n.toLocaleString("en")} downloads`,
  buildFromSource: "Prefer to build it yourself? Instructions are on GitHub.",
  openSource: "Open source",
  share: "Share",
  linkCopied: "Link copied",
  shareText: "Type Kazakh on a Russian keyboard: KazType adds the Kazakh letters for you",
  madeBy: "Made by",
  authorRole: "AI/ML engineer, former designer",
  donate: "Support the project",
  donateKaspi: "Kaspi transfer",
  numberCopied: "Number copied",
  license: "MIT license. Contributions are welcome.",
  footer: "The demo runs entirely in your browser. Nothing you type is sent anywhere.",
  convertFrom: "Text without Kazakh letters",
  convertTo: "Fixed text",
  convertPlaceholder: "Paste text typed without Kazakh letters…",
  copy: "Copy",
  copied: "Copied",
  padLabel: "Type Kazakh with Russian letters",
  padPlaceholder: "Click and type in Russian letters…",
  keyChoose: "choose",
  keyPick: "pick",
  keyCommit: "commit",
  keyEscape: "keep as typed",
  onDevice: "ms · on-device",
  clear: "Clear",
  try: "Try:",
  videoLabel: "KazType typing one Kazakh sentence in WhatsApp, Gmail, Word and Chrome",
  padCta: "Click and try it out right here",
  tryTitle: "Try it yourself",
  tryText: "Right here in the browser, nothing to install.",
  keyboardHint: "Kazakh letters hide in these eight keys. Press к and you get қ when the word needs it.",
  benefitsTitle: "Kazakh letters without hunting or switching",
  benefitsText: "KazType fixes the three things that make typing Kazakh slow.",
  benefits: [
    {
      pain: "The letters aren't on the keys",
      title: "No hunting for letters",
      text: "Ә, Ғ, Қ, Ң, Ө, Ұ, Ү, Һ, І aren't printed on the keyboard, and with KazType they don't need to be. Press к, and you get қ wherever the word needs it.",
    },
    {
      pain: "A different layout to learn",
      title: "Nothing new to learn",
      text: "You stay on the Russian layout. Digits, punctuation and muscle memory stay where they are, and there is no layout to switch to.",
    },
    {
      pain: "Text typed without Kazakh letters",
      title: "Correct text right away",
      text: "жексенби куни comes out as жексенбі күні. When a word can be read two ways, like қол and көл, you pick the right one with a single digit.",
    },
  ],
  trickyTitle: "Try words people usually get wrong",
  trickyHint: "Click one and it is typed in the demo above",
  noKeyboard: "No Russian keyboard? Type anyway. Keys are read by position on the ЙЦУКЕН layout, as KazType does.",
}

export type Dict = typeof en

const kk: Dict = {
  metaDescription: "Кәдімгі орыс пернетақтасында қазақша теріңіз. KazType ә, ғ, қ, ң, ө, ұ, ү, һ, і әріптерін теру кезінде өзі қояды. Windows және macOS үшін.",
  language: "Тіл",
  nav: { demo: "Демо", why: "Артықшылықтары", download: "Жүктеу", theme: "Тақырыпты ауыстыру" },
  heroLead: "Әдеттегідей теріңіз. ",
  heroAccent: <><FlipWord typed="Казакшаныз" fixed="Қазақшаңыз" holdMs={4500} /> дұрыс шығады.</>,
  heroText: (
    <>
      Кәдімгі орыс пернетақтасында <FlipWord typed="жексенби куни" fixed="жексенбі күні" startMs={1600} className="font-medium" /> деп теріңіз, қазақ әріптері теру барысында өзі қойылады.
    </>
  ),
  tryNow: "Қазір байқап көріңіз",
  download: "Жүктеу",
  loadError: "Модельді жүктеу мүмкін болмады:",
  loading: "Модель жүктелуде (2,6 МБ)…",
  tabType: "Теру",
  tabConvert: "Дайын мәтінді түзету",
  getTitle: "KazType орнату",
  getText: (
    <>
      Теруге болатын барлық жерде жұмыс істейді: браузер, Word, Telegram, WhatsApp. Сөздің керек нұсқасын{" "}
      {mono("1–9")} сандарымен таңдайсыз, KazType таңдауыңызды есте сақтайды.
    </>
  ),
  winTitle: "Windows 10 және 11",
  winDesc: "Кез келген Windows компьютеріне: x64, x86 және ARM64.",
  winStep1: "Орнатқышты іске қосыңыз. Ол әлі қол қойылмаған, сондықтан SmartScreen ескертсе, «Подробнее» > «Выполнить в любом случае» таңдаңыз.",
  winStep2: <>{mono("Win + Space")} басып, «Kazakh – KazType» таңдаңыз.</>,
  macDesc: "Apple silicon және Intel негізіндегі Mac үшін.",
  macStep1: "Орнатқышты ашыңыз. Ол әлі нотариаттан өтпеген, сондықтан macOS бөгесе, System Settings > Privacy & Security ашып, Open Anyway басыңыз.",
  macStep2: "Енгізу мәзірінен «Қазақша (KazType)» таңдаңыз. Ол көрінбесе, жүйеден шығып, қайта кіріңіз.",
  downloadFile: (ext: string) => `${ext} жүктеу`,
  version: (v: string) => `${v} нұсқасы`,
  downloads: (n: number) => `${n.toLocaleString("ru")} рет жүктелді`,
  buildFromSource: "Өзіңіз құрастырғыңыз келе ме? Нұсқаулық GitHub-та.",
  openSource: "Ашық бастапқы код",
  share: "Бөлісу",
  linkCopied: "Сілтеме көшірілді",
  shareText: "Орыс пернетақтасында қазақша теріңіз: KazType қазақ әріптерін өзі қояды",
  madeBy: "Авторы",
  authorRole: "AI/ML инженері, бұрынғы дизайнер",
  donate: "Жобаны қолдау",
  donateKaspi: "Kaspi аударымы",
  numberCopied: "Нөмір көшірілді",
  license: "MIT лицензиясы. Үлес қосуға қуаныштымыз.",
  footer: "Демо толығымен браузеріңізде жұмыс істейді. Терген мәтініңіз ешқайда жіберілмейді.",
  convertFrom: "Қазақ әріптерінсіз мәтін",
  convertTo: "Түзетілген мәтін",
  convertPlaceholder: "Қазақ әріптерінсіз терілген мәтінді қойыңыз…",
  copy: "Көшіру",
  copied: "Көшірілді",
  padLabel: "Орыс әріптерімен қазақша теріңіз",
  padPlaceholder: "Басып, орыс әріптерімен теріңіз…",
  keyChoose: "ауысу",
  keyPick: "таңдау",
  keyCommit: "растау",
  keyEscape: "терілгендей қалдыру",
  onDevice: "мс · құрылғыда",
  clear: "Тазалау",
  try: "Байқап көріңіз:",
  videoLabel: "KazType бір қазақша сөйлемді WhatsApp, Gmail, Word және Chrome-да тереді",
  padCta: "Басып, дәл осында байқап көріңіз",
  tryTitle: "Өзіңіз байқап көріңіз",
  tryText: "Дәл осында, браузерде, орнатусыз.",
  keyboardHint: "Қазақ әріптері осы сегіз пернеде жасырынған. к басасыз, сөзге керек болса қ шығады.",
  benefitsTitle: "Қазақ әріптері іздеусіз және ауыстырусыз",
  benefitsText: "KazType қазақша теруді баяулататын үш мәселені шешеді.",
  benefits: [
    {
      pain: "Пернелерде әріп жоқ",
      title: "Әріп іздеудің қажеті жоқ",
      text: "Ә, Ғ, Қ, Ң, Ө, Ұ, Ү, Һ, І пернетақтада салынбаған, бірақ KazType-пен оларды іздемейсіз. к басасыз, сөзге керек жерде қ шығады.",
    },
    {
      pain: "Басқа орналасуға үйрену керек",
      title: "Қайта үйренудің қажеті жоқ",
      text: "Орыс орналасуында қаласыз. Сандар, тыныс белгілері және саусақтарыңыздың әдеті өз орнында, орналасуды ауыстырудың керегі жоқ.",
    },
    {
      pain: "Мәтін қазақ әріптерінсіз жазылады",
      title: "Мәтін бірден сауатты",
      text: "«жексенби куни» орнына «жексенбі күні» шығады. Сөзді екі түрлі оқуға болса, мысалы қол мен көл, керегін бір санмен таңдайсыз.",
    },
  ],
  trickyTitle: "Жиі қате жазылатын сөздерді байқап көріңіз",
  trickyHint: "Басыңыз, сөз жоғарыдағы демоға теріледі",
  noKeyboard:
    "Орыс пернетақтасы жоқ па? Бәрібір тере беріңіз. KazType сияқты, пернелер ЙЦУКЕН орналасуындағы орнына қарай оқылады.",
}

const ru: Dict = {
  metaDescription: "Печатайте по-казахски на обычной русской клавиатуре. KazType сам ставит ә, ғ, қ, ң, ө, ұ, ү, һ, і во время набора. Для Windows и macOS.",
  language: "Язык",
  nav: { demo: "Демо", why: "Преимущества", download: "Скачать", theme: "Сменить тему" },
  heroLead: "Печатайте как привыкли. ",
  heroAccent: <>Получайте <FlipWord typed="казакша" fixed="қазақша" holdMs={4500} />.</>,
  heroText: (
    <>
      Наберите <FlipWord typed="жексенби куни" fixed="жексенбі күні" startMs={1600} className="font-medium" /> на обычной русской раскладке, и казахские буквы встанут сами, прямо во время набора.
    </>
  ),
  tryNow: "Попробовать",
  download: "Скачать",
  loadError: "Не удалось загрузить модель:",
  loading: "Загрузка модели (2,6 МБ)…",
  tabType: "Печатать",
  tabConvert: "Исправить готовый текст",
  getTitle: "Установить KazType",
  getText: (
    <>
      Работает везде, где можно печатать: в браузере, Word, Telegram, WhatsApp. Нужный вариант слова выбирается цифрой{" "}
      {mono("1–9")}, и KazType запоминает ваш выбор.
    </>
  ),
  winTitle: "Windows 10 и 11",
  winDesc: "Для любого компьютера с Windows: x64, x86 и ARM64.",
  winStep1: "Запустите установщик. Он пока не подписан, поэтому если SmartScreen предупредит, выберите «Подробнее» > «Выполнить в любом случае».",
  winStep2: <>Нажмите {mono("Win + Space")} и выберите «Kazakh – KazType».</>,
  macDesc: "Для Mac на Apple silicon и Intel.",
  macStep1: "Откройте установщик. Он пока не нотаризован, поэтому если macOS его заблокирует, откройте System Settings > Privacy & Security и нажмите Open Anyway.",
  macStep2: "Выберите «Қазақша (KazType)» в меню ввода. Если его нет, выйдите из системы и войдите снова.",
  downloadFile: (ext: string) => `Скачать ${ext}`,
  version: (v: string) => `Версия ${v}`,
  downloads: (n: number) => `Скачиваний: ${n.toLocaleString("ru")}`,
  buildFromSource: "Хотите собрать сами? Инструкция на GitHub.",
  openSource: "Открытый исходный код",
  share: "Поделиться",
  linkCopied: "Ссылка скопирована",
  shareText: "Печатайте по-казахски на русской клавиатуре: KazType сам ставит казахские буквы",
  madeBy: "Автор",
  authorRole: "AI/ML-инженер, бывший дизайнер",
  donate: "Поддержать проект",
  donateKaspi: "Перевод на Kaspi",
  numberCopied: "Номер скопирован",
  license: "Лицензия MIT. Будем рады вашему вкладу.",
  footer: "Демо работает целиком в браузере. Набранный текст никуда не отправляется.",
  convertFrom: "Текст без казахских букв",
  convertTo: "Исправленный текст",
  convertPlaceholder: "Вставьте текст, набранный без казахских букв…",
  copy: "Копировать",
  copied: "Скопировано",
  padLabel: "Набирайте по-казахски русскими буквами",
  padPlaceholder: "Нажмите и печатайте русскими буквами…",
  keyChoose: "переключить",
  keyPick: "выбрать",
  keyCommit: "подтвердить",
  keyEscape: "оставить как набрано",
  onDevice: "мс · на устройстве",
  clear: "Очистить",
  try: "Попробуйте:",
  videoLabel: "KazType набирает одно казахское предложение в WhatsApp, Gmail, Word и Chrome",
  padCta: "Нажмите и попробуйте прямо здесь",
  tryTitle: "Попробуйте сами",
  tryText: "Прямо здесь, в браузере, без установки.",
  keyboardHint: "Казахские буквы спрятаны в этих восьми клавишах. Нажимаете к и получаете қ, когда слово этого требует.",
  benefitsTitle: "Казахские буквы без поиска и переключений",
  benefitsText: "KazType решает три проблемы, из-за которых печатать по-казахски долго.",
  benefits: [
    {
      pain: "Букв нет на клавишах",
      title: "Не нужно искать буквы",
      text: "Ә, Ғ, Қ, Ң, Ө, Ұ, Ү, Һ, І не нарисованы на клавиатуре, и с KazType это не мешает. Нажимаете к, и там, где слово этого требует, появляется қ.",
    },
    {
      pain: "Нужно привыкать к другой раскладке",
      title: "Не нужно переучиваться",
      text: "Вы остаётесь в русской раскладке. Цифры, знаки препинания и привычные движения пальцев на своих местах, переключать раскладку не нужно.",
    },
    {
      pain: "Текст пишут без казахских букв",
      title: "Текст сразу грамотный",
      text: "Вместо «жексенби куни» получается «жексенбі күні». Если слово читается по-разному, как қол и көл, нужный вариант выбирается одной цифрой.",
    },
  ],
  trickyTitle: "Попробуйте слова, в которых обычно ошибаются",
  trickyHint: "Нажмите, и слово наберётся в демо выше",
  noKeyboard:
    "Нет русской раскладки? Всё равно печатайте. Клавиши считываются по их месту в раскладке ЙЦУКЕН, как в KazType.",
}

const DICTS: Record<Lang, Dict> = { kk, ru, en }

const isLang = (v: string | null): v is Lang => LANGS.some((l) => l.code === v)

/** `?lang=` wins, then the saved choice, then Kazakh. */
function initialLang(): Lang {
  const param = new URLSearchParams(window.location.search).get("lang")
  if (isLang(param)) return param
  try {
    const saved = localStorage.getItem("lang")
    if (isLang(saved)) return saved
  } catch { /* storage blocked */ }
  return DEFAULT_LANG
}

const I18n = createContext<{ lang: Lang; t: Dict; setLang: (l: Lang) => void } | null>(null)

export function I18nProvider({ children }: { children: ReactNode }) {
  const [lang, setLang] = useState<Lang>(initialLang)
  const t = DICTS[lang]

  useEffect(() => {
    document.documentElement.lang = lang
    document.querySelector('meta[name="description"]')?.setAttribute("content", t.metaDescription)
    try { localStorage.setItem("lang", lang) } catch { /* storage blocked */ }
    const url = new URL(window.location.href)
    if (lang === DEFAULT_LANG) url.searchParams.delete("lang")
    else url.searchParams.set("lang", lang)
    window.history.replaceState(null, "", url)
  }, [lang, t])

  return <I18n.Provider value={{ lang, t, setLang }}>{children}</I18n.Provider>
}

export function useI18n() {
  const ctx = useContext(I18n)
  if (!ctx) throw new Error("useI18n outside I18nProvider")
  return ctx
}
