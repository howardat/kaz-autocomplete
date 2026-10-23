GOAL:    the viewer sees one Kazakh sentence typed on a Russian keyboard come out correct in every app they use, and wants to try it.
OUTPUT:  HyperFrames, 1920 x 1080, 30 fps, silent, seamless loop for the KazType site hero (MP4 + WebM).
TIME:    15 s x 30 fps = 450 frames. No music: the site plays it muted.

LOOK:    the medium is the real apps on a Mac, filmed close: each app is drawn at its true desktop
         size (1x UI px) and the camera frames it at 1.9x, so body text lands at 28-34 px on the canvas.
         Ground is the app itself, never a void: WhatsApp #efeae2 chat wall, Gmail #ffffff on #f6f8fc,
         Word page #ffffff on #e9e9e9 canvas, Chrome #ffffff omnibox on #dee1e6 tab strip.
         Ink is each app's own: #111b21 (WhatsApp), #1f1f1f (Gmail), #000000 (Word), #1f1f1f (Chrome).
         Faces: SF Pro (WhatsApp, local), Roboto (Gmail, Chrome), Lato standing in for Aptos (Word).
         One accent, KazType's: the composing word's 2 px underline and the selected candidate row,
         #0a84ff (macOS selection blue). Nothing else in the frame uses it.
         Depth: edge vignette 18% black, radial, so the eye stays on the line; no blur, no glow.
OBJECTS: the text caret (2 px, app ink); the composing word, underlined as macOS marks it;
         the KazType candidate panel: white, 8 px radius, 0 8px 24px rgba(0,0,0,.18), rows "1 сөз",
         selected row #0a84ff with white text; keycaps of the raw Russian letters at the bottom,
         like a keystroke overlay in a screen recording, 44 px tall, #1d1d1f at 82%, white mono letters.
MOTION:  typing rhythm from the bake: about 4 keys a second with seeded jitter, a 0.42 s hold before
         each word commits so its candidates read. Camera per shot: one move, a 1.9x -> 1.97x push
         plus a pan that keeps the caret in the middle third, power1.inOut. Cut on a keystroke:
         the incoming app enters at 1.03x its framing and settles in 0.35 s expo.out.
SHOTS:   1. 0.0-3.6   WhatsApp, reply field under Aigerim's message. Reads: "Сәлем!" then
            "Жексен..." with the candidate panel. Cut mid-word on a keystroke.
         2. 3.6-7.2   Gmail compose body, same sentence continues: "...бі күні тауға".
         3. 7.2-10.8  Word page, "...барамыз, сендер".
         4. 10.8-15.0 Chrome address bar with suggestions, "...де келіңдер" lands, holds 1.2 s
            on the whole sentence, then cuts to the empty WhatsApp field (loop seam).
BANNED:  dark void ground; glow, bloom, gradients as decoration; app logos; text that drifts after it
         lands (the camera may move, the type never animates); crossfades between apps;
         everything centred; fake UI copy in English.
QA:      t=1.6 (Сәлем + candidates), 3.55/3.65 (seam 1), 5.4, 7.15/7.25 (seam 2), 9.0,
         10.75/10.85 (seam 3), 13.8 (full sentence), 14.95 and 0.0 (loop seam).
