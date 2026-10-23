# KazType

An input method for macOS and Windows for typing Kazakh with a Russian keyboard. Type `казакстан`
and get `қазақстан`. A small on-device neural network restores the Kazakh letters:

| Typed | Can become |
|---|---|
| к | к, қ |
| г | г, ғ |
| н | н, ң |
| о | о, ө |
| у | у, ұ, ү |
| а | а, ә |
| и | и, і |
| х | х, һ |

The mapping lives in `transliteration_rules.yaml`.

## Using it

1. Switch to **Қазақша (KazType)** in the input menu.
2. Type with Russian letters. The current word shows underlined, already converted.
3. **Space** or punctuation commits the top choice. **Return** commits without adding a newline.
4. **↑/↓** or **1–9** pick another candidate. **Esc** commits exactly what you typed.
5. When you pick something other than the model's first guess, KazType remembers it
   (`~/Library/Application Support/KazType/user_dict.json`).

Keys are read with the Russian layout you used before switching. If none is found, it uses Russian - PC.

## Layout

```
transliteration_rules.yaml  letter mapping (source of truth)
ml/                         Python: data, training, eval, Core ML and Windows export
macos/                      Swift: KazCore (conversion, tested) + KazType (IMK app)
windows/                    C++: core (conversion + int8 BiLSTM, tested) + tsf (TSF text service DLL)
web/                        Vite + React + shadcn demo site; runs windows/core as WebAssembly
```

## Train the model

```sh
uv run python -m ml.data                # ~150M chars from kz-transformers/multidomain-kazakh-dataset -> data/
uv run python -m ml.train               # char BiLSTM tagger on MPS -> data/model.pt
uv run python -m ml.export              # -> macos/Resources/{KazTagger.mlpackage,vocab.json}
uv run python -m ml.eval --coreml       # accuracy + latency vs targets
uv run python -m ml.export_win          # -> windows/Resources/kaztype.bin (+ accuracy of its int8 math)
```

Targets: ≥97% top-1 and ≥99.5% top-3 word accuracy on held-out words with ambiguous
letters, <10 ms per conversion, model <5 MB.

## Build and install

Needs only Xcode Command Line Tools.

```sh
cd macos
make test       # unit tests (real-model tests run after ml.export)
make install    # build, ad-hoc sign, copy to ~/Library/Input Methods, register + enable
make uninstall
```

If the input source does not appear after install, log out and back in.

## Windows

A Text Services Framework input method under Kazakh, with Russian as its
substitute layout. Same keys as macOS. The model runs in plain C++ with int8
weights and activations (no runtime dependencies); its accuracy matches the float
model. The user dictionary is `%APPDATA%\KazType\user_dict.tsv`.

To install, run `KazType-Setup-<version>.exe` (from the `KazType-Setup` artifact of the
`windows` workflow). It asks for administrator rights, and its uninstaller is in
Settings > Apps. It is not code-signed, so SmartScreen warns: choose More info > Run anyway.

To build, you need Visual Studio 2022 (Desktop C++) and CMake. The installer also needs
Inno Setup 6.3+ (`winget install JRSoftware.InnoSetup`). In PowerShell:

```powershell
cd windows
.\build.ps1                                                  # x64, x86, arm64 + core tests + installer -> dist\
powershell -ExecutionPolicy Bypass -File dist\install.ps1    # as administrator
powershell -ExecutionPolicy Bypass -File dist\uninstall.ps1
```

Switch with **Win+Space** to "Kazakh – KazType". Apps that were open before
the install pick it up after a restart. x64 apps emulated on ARM64 are not supported.

The core and its tests also build on macOS/Linux:

```sh
cd windows && cmake -B build && cmake --build build && ctest --test-dir build
```

## Web demo

```sh
cd web
sh wasm/build.sh   # compile windows/core to WebAssembly (Docker, emscripten/emsdk)
npm install && npm run dev
```
