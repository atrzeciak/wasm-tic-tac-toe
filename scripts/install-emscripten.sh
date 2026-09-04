#!/bin/bash
# Installs the Emscripten SDK into EMSDK_DIR and prebuilds the SDL2 ports the
# game links. Used by .devcontainer/Dockerfile (baked into the image) and by
# CI (cached in $HOME/emsdk).
set -euo pipefail
set -x

EMSDK_DIR="${EMSDK_DIR:-${PWD}/emsdk}"
# Pin the toolchain so the devcontainer image and CI build identically.
# Use "latest" to track upstream instead.
EMSDK_VERSION="${EMSDK_VERSION:-latest}"

git clone "https://github.com/emscripten-core/emsdk.git" --depth 1 "${EMSDK_DIR}"
"${EMSDK_DIR}/emsdk" install "${EMSDK_VERSION}"
"${EMSDK_DIR}/emsdk" activate "${EMSDK_VERSION}"

# shellcheck disable=SC1091
source "${EMSDK_DIR}/emsdk_env.sh"

# Only the ports this project links (see CMakeLists.txt): SDL2, SDL2_ttf and
# SDL2_image with PNG support.
embuilder.py build sdl2 sdl2_ttf sdl2_image-png

# Let every user build further ports or system libraries into the cache
# (emcc does this on demand); ports/ and others are created root-owned.
chmod -R a+rwX "${EMSDK_DIR}/upstream/emscripten/cache"
