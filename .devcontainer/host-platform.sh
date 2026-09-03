#!/bin/sh
# Writes .devcontainer/.env with HOST_PLATFORM=linux/<arch> of the Docker
# engine, so docker-compose.yml builds and runs the image natively (arm64 on
# Apple Silicon, amd64 on Intel) even when DOCKER_DEFAULT_PLATFORM=linux/amd64
# is exported in the shell. An emulated amd64 container under Rosetta has no
# ptrace support (gdb fails) and is slower. Run automatically by the Makefile
# and by devcontainer.json's initializeCommand; safe to run by hand.
set -e

dir=$(cd "$(dirname "$0")" && pwd)

# Prefer the engine's arch (correct even with a remote DOCKER_HOST); fall back
# to the local machine if docker isn't reachable yet.
arch=$(docker info --format '{{.Architecture}}' 2>/dev/null || uname -m)
case "$arch" in
  x86_64 | amd64) platform=linux/amd64 ;;
  aarch64 | arm64) platform=linux/arm64 ;;
  *)
    echo "host-platform.sh: unsupported architecture '$arch'" >&2
    exit 1
    ;;
esac

printf 'HOST_PLATFORM=%s\n' "$platform" > "$dir/.env"
echo "HOST_PLATFORM=$platform"
