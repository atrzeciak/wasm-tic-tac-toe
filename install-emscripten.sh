#!/bin/bash

set -e
set -x

EMSDK_DIR="${EMSDK_DIR:-${PWD}/emsdk}"
# Pin the toolchain so the devcontainer image and CI build identically.
# Use "latest" to track upstream instead.
EMSDK_VERSION="${EMSDK_VERSION:-latest}"

git clone "https://github.com/emscripten-core/emsdk.git" --depth 1 "${EMSDK_DIR}"
"${EMSDK_DIR}/emsdk" install "${EMSDK_VERSION}"
"${EMSDK_DIR}/emsdk" activate "${EMSDK_VERSION}"

chmod -R 777 "${EMSDK_DIR}/upstream/emscripten/cache"

# shellcheck disable=SC1091
source "${EMSDK_DIR}/emsdk_env.sh"

# Only the ports this project links (see CMakeLists.txt).
embuilder.py build sdl2 sdl2_ttf
