#!/bin/bash

set -e
set -x

EMSDK_DIR="${EMSDK_DIR:-${PWD}/emsdk}"

git clone "https://github.com/emscripten-core/emsdk.git" --depth 1 "${EMSDK_DIR}"
"${EMSDK_DIR}/emsdk" install latest
"${EMSDK_DIR}/emsdk" activate latest

chmod -R 777 "${EMSDK_DIR}/upstream/emscripten/cache"

# shellcheck disable=SC1091
source "${EMSDK_DIR}/emsdk_env.sh"

embuilder.py build sdl2_ttf sdl2_image sdl2_net sdl2
