FROM mcr.microsoft.com/devcontainers/cpp:1-ubuntu-24.04

# python3 runs scripts/convert.py during the build (see CMakeLists.txt) and
# is what emsdk uses on Linux.
RUN apt-get update \
 && apt-get install -y --no-install-recommends \
    libsdl2-dev \
    libsdl2-ttf-dev \
    libsdl2-image-dev \
    ninja-build \
    python3 \
    clang-format \
    clang-tidy \
    shellcheck \
 && rm -rf /var/lib/apt/lists/*

# Bake the emscripten toolchain and its prebuilt SDL2 ports into the image so
# builds never depend on a host-side emsdk checkout.
ARG EMSDK_VERSION=6.0.9
COPY scripts/install-emscripten.sh /tmp/install-emscripten.sh
RUN EMSDK_DIR=/opt/emsdk EMSDK_VERSION="${EMSDK_VERSION}" bash /tmp/install-emscripten.sh \
 && rm /tmp/install-emscripten.sh

# Enable bash completion for every user (make targets, cmake, git, ...).
# The package ships with the base image but is only wired up in the vscode
# user's .bashrc; this turns on the system-wide hook in /etc/bash.bashrc.
RUN sed -i '/^# enable bash completion in interactive shells/,/^#fi/{s/^#\(if\|  if\|    \.\|  elif\|  fi\|fi\)/\1/}' /etc/bash.bashrc

# docker-compose.yml mounts a named volume here to keep the Claude Code login
# across rebuilds. A new volume copies this directory's ownership; without it
# Docker creates the mountpoint as root and the vscode user cannot write it.
RUN mkdir -p /home/vscode/.claude && chown vscode:vscode /home/vscode/.claude
