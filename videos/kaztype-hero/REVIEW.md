# Review: KazType hero (15 s loop)

- Creative calls: one sentence typed without a break across WhatsApp, Gmail, Word and Chrome, on the Studio look (windows as tilted cards) over the "lagoon" gradient (soft turquoise-blue fields with grain against banding). The caret is pinned at (1250, 660) in every shot and the camera glides after it (averaged over 0.45 s). Apps change on a keystroke mid-word with a hard cut; the new app enters 1.8% large and settles over 0.6 s. Letters KazType turns Kazakh flip in 0.6 s, hold in #0a84ff and fade to ink; their keys in the dark keystroke overlay wear the Kazakh letter as a badge. A translucent dock at the top lights the current app; its caption is one line for the whole film. Typing comes from the real engine (`scripts/bake-typing.mjs`). Silent by choice.
- Languages: one render per site language via `--variables '{"everywhere-line": "..."}'` (EN "Works everywhere you need", KK "Қажет жердің бәрінде жұмыс істейді", RU "Работает везде, где вам нужно").
- Measured: `check` passes; snapshots at every shot and the final; rendered files 15.000 s, 1920x1080, 30 fps. Not checked: the loop played back in a browser.
- Kept on purpose: contrast warnings on grey UI captions inside the apps; text occluded by the dock at the top edge of the windows.
- Fonts: SF Pro and Times New Roman come from this Mac (`local()`); a render elsewhere falls back to Inter/serif.
- Variables keep the other explored designs (looks desktop/night/macbook, other grounds, other badge styles) for later iterations; `scripts/set-default.py` switches their defaults.
