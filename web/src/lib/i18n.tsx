import { createContext, useContext, useEffect, useState, type ReactNode } from "react"

export const LANGS = [
  { code: "kk", label: "Қаз" },
  { code: "ru", label: "Рус" },
  { code: "en", label: "Eng" },
] as const

export type Lang = (typeof LANGS)[number]["code"]

const DEFAULT_LANG: Lang = "kk"

const b = (s: string) => <span className="font-medium text-foreground">{s}</span>
const mono = (s: string) => <span className="font-mono">{s}</span>

const en = {
  metaDescription: "Type Kazakh on a Russian keyboard. An on-device neural input method for macOS and Windows.",
  language: "Language",
  nav: { demo: "Demo", how: "How it works", download: "Download", theme: "Toggle theme" },
  badge: "On-device neural input method · macOS & Windows",
  heroLead: "Type Kazakh on a ",
  heroAccent: "Russian keyboard.",
  heroText: (
    <>
      Type {b("казакстан")}, get {b("қазақстан")}. A tiny neural network restores ә ғ қ ң ө ұ ү һ і as you type, right on
      your machine.
    </>
  ),
  tryNow: "Try it now",
  download: "Download",
  loadError: "Could not load the model:",
  loading: "Loading the model (2.6 MB)…",
  tabType: "Type live",
  tabConvert: "Convert text",
  howTitle: "Eight keys, nine missing letters",
  howText:
    "A Russian layout has no key for nine Kazakh letters. KazType reads each typed letter in context and picks the Kazakh one you meant.",
  stats: [
    { value: "99.0%", label: "top-1 accuracy", detail: "on held-out words with ambiguous letters" },
    { value: "99.9%", label: "in the top 3", detail: "the right word is almost always one key away" },
    { value: "<10 ms", label: "per word", detail: "a 2-layer BiLSTM, int8, on your CPU" },
    { value: "2.6 MB", label: "model", detail: "nothing leaves your machine" },
  ],
  getTitle: "Get KazType",
  getText: (
    <>
      Works in every app: browsers, Word, Telegram, terminals. Pick a candidate with {mono("1–9")}; your choices are
      remembered.
    </>
  ),
  winTitle: "Windows 10 & 11",
  winDesc: "A Text Services Framework input method. x64, x86 and ARM64.",
  winStep1: "Build with Visual Studio 2022 and CMake:",
  winStep2: "Install from an administrator PowerShell:",
  winStep3: <>Press {mono("Win + Space")} and choose “Kazakh – KazType”.</>,
  macDesc: "An Input Method Kit app with a Core ML model. Apple silicon and Intel.",
  macStep1: "Build and install (needs only Xcode Command Line Tools):",
  macStep2: "Choose “Қазақша (KazType)” in the input menu. If it is missing, log out and back in.",
  installersSoon: "Prebuilt installers are coming soon.",
  footer: "The demo runs entirely in your browser. Nothing you type is sent anywhere.",
  convertFrom: "Russian keyboard text",
  convertTo: "Kazakh",
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
  noKeyboard: "No Russian keyboard? Type anyway. Keys are read by position on the ЙЦУКЕН layout, as KazType does.",
}

export type Dict = typeof en

const kk: Dict = {
  metaDescription: "Орыс пернетақтасында қазақша жазыңыз. macOS және Windows үшін құрылғыда жұмыс істейтін нейрондық енгізу әдісі.",
  language: "Тіл",
  nav: { demo: "Демо", how: "Қалай жұмыс істейді", download: "Жүктеу", theme: "Тақырыпты ауыстыру" },
  badge: "Құрылғыда жұмыс істейтін нейрондық енгізу әдісі · macOS және Windows",
  heroLead: "Орыс пернетақтасында ",
  heroAccent: "қазақша жазыңыз.",
  heroText: (
    <>
      {b("казакстан")} деп теріңіз, {b("қазақстан")} шығады. Шағын нейрондық желі ә ғ қ ң ө ұ ү һ і әріптерін теру
      барысында, тікелей компьютеріңізде қалпына келтіреді.
    </>
  ),
  tryNow: "Қазір байқап көріңіз",
  download: "Жүктеу",
  loadError: "Модельді жүктеу мүмкін болмады:",
  loading: "Модель жүктелуде (2,6 МБ)…",
  tabType: "Тікелей теру",
  tabConvert: "Мәтінді түрлендіру",
  howTitle: "Сегіз перне, тоғыз жетіспейтін әріп",
  howText:
    "Орыс пернетақтасында тоғыз қазақ әрпіне перне жоқ. KazType әр терілген әріпті контекске қарай оқып, сіз ойлаған қазақ әрпін таңдайды.",
  stats: [
    { value: "99,0%", label: "top-1 дәлдік", detail: "екіұшты әріптері бар тестілік сөздерде" },
    { value: "99,9%", label: "алғашқы 3 нұсқада", detail: "дұрыс сөз әрдайым дерлік бір перне басу қашықтығында" },
    { value: "<10 мс", label: "бір сөзге", detail: "2 қабатты BiLSTM, int8, сіздің процессорыңызда" },
    { value: "2,6 МБ", label: "модель", detail: "ешбір дерек құрылғыдан шықпайды" },
  ],
  getTitle: "KazType орнату",
  getText: (
    <>
      Барлық қолданбада жұмыс істейді: браузерлер, Word, Telegram, терминалдар. Нұсқаны {mono("1–9")} пернелерімен
      таңдаңыз; таңдауларыңыз есте сақталады.
    </>
  ),
  winTitle: "Windows 10 және 11",
  winDesc: "Text Services Framework негізіндегі енгізу әдісі. x64, x86 және ARM64.",
  winStep1: "Visual Studio 2022 және CMake арқылы құрастырыңыз:",
  winStep2: "Әкімші құқығымен ашылған PowerShell-де орнатыңыз:",
  winStep3: <>{mono("Win + Space")} басып, «Kazakh – KazType» таңдаңыз.</>,
  macDesc: "Core ML моделі бар Input Method Kit қолданбасы. Apple silicon және Intel.",
  macStep1: "Құрастырып, орнатыңыз (тек Xcode Command Line Tools қажет):",
  macStep2: "Енгізу мәзірінен «Қазақша (KazType)» таңдаңыз. Ол көрінбесе, жүйеден шығып, қайта кіріңіз.",
  installersSoon: "Дайын орнатқыштар жақында шығады.",
  footer: "Демо толығымен браузеріңізде жұмыс істейді. Терген мәтініңіз ешқайда жіберілмейді.",
  convertFrom: "Орыс пернетақтасымен терілген мәтін",
  convertTo: "Қазақша",
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
  noKeyboard:
    "Орыс пернетақтасы жоқ па? Бәрібір тере беріңіз. KazType сияқты, пернелер ЙЦУКЕН орналасуындағы орнына қарай оқылады.",
}

const ru: Dict = {
  metaDescription: "Пишите по-казахски на русской клавиатуре. Нейросетевой метод ввода для macOS и Windows, работающий на устройстве.",
  language: "Язык",
  nav: { demo: "Демо", how: "Как это работает", download: "Скачать", theme: "Сменить тему" },
  badge: "Нейросетевой метод ввода на устройстве · macOS и Windows",
  heroLead: "Пишите по-казахски ",
  heroAccent: "на русской клавиатуре.",
  heroText: (
    <>
      Наберите {b("казакстан")} и получите {b("қазақстан")}. Небольшая нейросеть восстанавливает ә ғ қ ң ө ұ ү һ і прямо
      во время набора, на вашем компьютере.
    </>
  ),
  tryNow: "Попробовать",
  download: "Скачать",
  loadError: "Не удалось загрузить модель:",
  loading: "Загрузка модели (2,6 МБ)…",
  tabType: "Набор вживую",
  tabConvert: "Конвертировать текст",
  howTitle: "Восемь клавиш, девять недостающих букв",
  howText:
    "В русской раскладке нет клавиш для девяти казахских букв. KazType смотрит на каждую набранную букву в контексте и выбирает казахскую, которую вы имели в виду.",
  stats: [
    { value: "99,0%", label: "точность top-1", detail: "на тестовых словах с неоднозначными буквами" },
    { value: "99,9%", label: "в первой тройке", detail: "нужное слово почти всегда в одном нажатии" },
    { value: "<10 мс", label: "на слово", detail: "двухслойная BiLSTM, int8, на вашем процессоре" },
    { value: "2,6 МБ", label: "модель", detail: "данные не покидают ваш компьютер" },
  ],
  getTitle: "Установить KazType",
  getText: (
    <>
      Работает во всех приложениях: браузерах, Word, Telegram, терминалах. Выбирайте вариант клавишами {mono("1–9")};
      ваш выбор запоминается.
    </>
  ),
  winTitle: "Windows 10 и 11",
  winDesc: "Метод ввода на основе Text Services Framework. x64, x86 и ARM64.",
  winStep1: "Соберите с помощью Visual Studio 2022 и CMake:",
  winStep2: "Установите из PowerShell, запущенного от имени администратора:",
  winStep3: <>Нажмите {mono("Win + Space")} и выберите «Kazakh – KazType».</>,
  macDesc: "Приложение Input Method Kit с моделью Core ML. Apple silicon и Intel.",
  macStep1: "Соберите и установите (нужны только Xcode Command Line Tools):",
  macStep2: "Выберите «Қазақша (KazType)» в меню ввода. Если его нет, выйдите из системы и войдите снова.",
  installersSoon: "Готовые установщики скоро появятся.",
  footer: "Демо работает целиком в браузере. Набранный текст никуда не отправляется.",
  convertFrom: "Текст с русской клавиатуры",
  convertTo: "По-казахски",
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
