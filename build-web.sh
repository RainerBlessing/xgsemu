#!/bin/sh
# Builds the browser version (WebAssembly) into build-web/.
# Needs the Emscripten SDK: source /path/to/emsdk/emsdk_env.sh
# Serve it with: python3 -m http.server -d build-web
set -e
command -v emcmake >/dev/null || { echo "emcmake not found, source emsdk_env.sh first"; exit 1; }
emcmake cmake -S . -B build-web -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF
cmake --build build-web --target xgsemu
cp build-web/xgsemu.html build-web/index.html
echo "Done: build-web/index.html"
