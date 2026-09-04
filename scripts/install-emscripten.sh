#!/bin/bash
# Installs the Emscripten SDK into EMSDK_DIR and prebuilds the SDL2 ports the
# game links. Used by .devcontainer/Dockerfile (baked into the image) and by
# CI (cached in $HOME/emsdk).
set -euo pipefail
set -x

# Copied into lowercase names because emsdk_env.sh unsets every EMSDK_*
# variable it finds when sourced below.
emsdk_dir="${EMSDK_DIR:-${PWD}/emsdk}"
# Pin the toolchain so the devcontainer image and CI build identically.
# Use "latest" to track upstream instead.
emsdk_version="${EMSDK_VERSION:-latest}"

git clone "https://github.com/emscripten-core/emsdk.git" --depth 1 "${emsdk_dir}"
"${emsdk_dir}/emsdk" install "${emsdk_version}"
"${emsdk_dir}/emsdk" activate "${emsdk_version}"

# shellcheck disable=SC1091
source "${emsdk_dir}/emsdk_env.sh"

# Only the ports this project links (see CMakeLists.txt): SDL2, SDL2_ttf and
# SDL2_image with PNG support.
embuilder.py build sdl2 sdl2_ttf sdl2_image-png

# Let every user build further ports or system libraries into the cache
# (emcc does this on demand); ports/ and others are created root-owned.
chmod -R a+rwX "${emsdk_dir}/upstream/emscripten/cache"
