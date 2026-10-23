#!/bin/sh
# Compile windows/core to WebAssembly for the web demo, via the emsdk image.
#   sh wasm/build.sh   (from web/)  ->  src/wasm/kaz.js, src/wasm/kaz.wasm, public/kaztype.bin
set -e
cd "$(dirname "$0")/../.."
mkdir -p web/src/wasm
docker run --rm -v "$PWD":/src -w /src -u "$(id -u):$(id -g)" emscripten/emsdk:latest \
  em++ -std=c++17 -O3 -msimd128 -Iwindows/core \
    web/wasm/kaz.cpp windows/core/Converter.cpp windows/core/Tagger.cpp windows/core/Text.cpp \
    windows/core/UserDict.cpp windows/core/Vocab.cpp \
    -sMODULARIZE -sEXPORT_ES6 -sENVIRONMENT=web -sALLOW_MEMORY_GROWTH -sFORCE_FILESYSTEM \
    -sEXPORTED_RUNTIME_METHODS=cwrap,FS \
    -o web/src/wasm/kaz.js
cp windows/Resources/kaztype.bin web/public/kaztype.bin
ls -la web/src/wasm web/public/kaztype.bin
