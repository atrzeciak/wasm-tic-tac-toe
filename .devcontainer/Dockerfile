FROM mcr.microsoft.com/devcontainers/cpp:1-ubuntu-24.04

RUN apt-get update \
 && apt-get install -y \
 libsdl2-dev \
 libsdl2-ttf-dev \
 ninja-build \
 clang-format \
 clang-tidy \
 shellcheck

# Bake the emscripten toolchain and its prebuilt SDL2 ports into the image so
# builds never depend on a host-side emsdk checkout.
ARG EMSDK_VERSION=6.0.9
COPY install-emscripten.sh /tmp/install-emscripten.sh
RUN EMSDK_DIR=/opt/emsdk EMSDK_VERSION="${EMSDK_VERSION}" bash /tmp/install-emscripten.sh

# Enable bash completion for every user (make targets, cmake, git, ...).
# The package ships with the base image but is only wired up in the vscode
# user's .bashrc; this turns on the system-wide hook in /etc/bash.bashrc.
RUN sed -i '/^# enable bash completion in interactive shells/,/^#fi/{s/^#\(if\|  if\|    \.\|  elif\|  fi\|fi\)/\1/}' /etc/bash.bashrc
