# -*- makefile -*-
# Recipes run under bash and stop on the first failing command, unset
# variable or broken pipeline; no implicit rules; half-built files are removed.
SHELL:=bash
.SHELLFLAGS:=-eu -o pipefail -c
.DELETE_ON_ERROR:
.SUFFIXES:
MAKEFLAGS+=--no-builtin-rules --warn-undefined-variables
.DEFAULT_GOAL:=all

# Environment inputs that are usually unset; defaulted so
# --warn-undefined-variables stays quiet.
REMOTE_CONTAINERS?=
DEVCONTAINER?=
DISPLAY?=

BUILD_TYPE?=Release
# Build trees are named cmake-bld-<kit>.local so they line up with the VS Code
# CMake Tools kits in .vscode/cmake-kits.json (see cmake.buildDirectory).
CMAKE_BUILD_DIR:=cmake-bld-Emscripten.local
# Native (non-wasm) tree: `make native` builds a Linux binary here, `make test`
# runs the unit tests from it, clang-tidy reads its compile database, and the
# IDE's "native" kit debugs it with gdb.
NATIVE_BUILD_DIR:=cmake-bld-native.local

# What the formatter and the linters look at.
SOURCES:=$(wildcard src/*.cpp src/*.h tests/*.cpp)
SHELL_SCRIPTS:=scripts/convert.sh scripts/install-emscripten.sh .devcontainer/host-platform.sh

WORKAREA:=/workarea
HTTP_PORT?=8000

# emsdk is baked into the image by the Dockerfile.
EMSDK_DIR?=/opt/emsdk
EMSCRIPTEN_CMAKE?="$(EMSDK_DIR)/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake"

# The native binary is a Linux build, so `make run` runs it in the container
# and opens its window on the host's X server.
#   macOS: XQuartz over TCP; it must accept network clients (`make xquartz-tcp`
#          once, then restart XQuartz).
#   Linux: the host's X socket is mounted into the container.
HOST_OS:=$(shell uname -s)
ifeq ($(HOST_OS),Darwin)
X11_DISPLAY?=host.docker.internal:0
X11_ALLOW:=xhost +localhost
X11_OPTS:=-e DISPLAY=$(X11_DISPLAY)
else
X11_DISPLAY?=$(DISPLAY)
X11_ALLOW:=xhost +local:docker
X11_OPTS:=-e DISPLAY=$(X11_DISPLAY) -v /tmp/.X11-unix:/tmp/.X11-unix
endif

# Detect whether make is already running inside the devcontainer. Any of:
#   - VS Code / devcontainer CLI export REMOTE_CONTAINERS or DEVCONTAINER
#   - Docker creates /.dockerenv in every container
#   - the repo is mounted at the container workspace path
# Override with IN_CONTAINER=1 or IN_CONTAINER=0 to force either mode.
ifeq ($(origin IN_CONTAINER),undefined)
  ifneq ($(filter true 1,$(REMOTE_CONTAINERS) $(DEVCONTAINER)),)
    IN_CONTAINER:=1
  else ifneq ($(wildcard /.dockerenv),)
    IN_CONTAINER:=1
  else ifeq ($(CURDIR),$(WORKAREA))
    IN_CONTAINER:=1
  else
    IN_CONTAINER:=0
  endif
endif

# $(RUN) prefixes every command that needs the toolchain.
#   inside the container: empty, commands run directly
#   on the host:          docker compose run, using the devcontainer's compose file
ifeq ($(IN_CONTAINER),1)
WORKAREA:=$(CURDIR)
RUN:=
SERVE:=
# VS Code forwards the host display into the devcontainer as $DISPLAY.
RUN_X11:=
else
# Compose lets an exported DOCKER_DEFAULT_PLATFORM override the service's
# `platform:` for builds and refuses when the two differ, so keep it away from
# every docker command make runs: the image is always the engine's own arch.
unexport DOCKER_DEFAULT_PLATFORM
# Pin the compose platform to the engine's arch (writes .devcontainer/.env).
$(if $(shell sh .devcontainer/host-platform.sh),,$(error host-platform.sh failed))
COMPOSE:=docker compose -f .devcontainer/docker-compose.yml
SERVICE:=wasm-tic-tac-toe
RUN:=$(COMPOSE) run --rm $(SERVICE)
SERVE:=$(COMPOSE) run --rm -p $(HTTP_PORT):$(HTTP_PORT) $(SERVICE)
RUN_X11:=$(X11_ALLOW) >/dev/null && $(COMPOSE) run --rm $(X11_OPTS) $(SERVICE)
endif

# Configure options shared by both trees. Configure runs before every build:
# it is a no-op when nothing changed, and it is the only way a different
# BUILD_TYPE takes effect (Ninja is single-config, so `cmake --build --config`
# cannot switch it).
CMAKE_CONFIGURE:=cmake \
  -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE \
  -DCMAKE_BUILD_TYPE:STRING=$(BUILD_TYPE) \
  -S$(WORKAREA) -G Ninja

.PHONY: all
all:

.PHONY: image
image:
ifeq ($(IN_CONTAINER),1)
	@echo "Already inside the devcontainer; run 'make image' on the host to rebuild the image."
else
	$(COMPOSE) build
endif

# ---- wasm ------------------------------------------------------------------

.PHONY: configure-wasm
configure-wasm:
	$(RUN) $(CMAKE_CONFIGURE) \
	  -DCMAKE_TOOLCHAIN_FILE=$(EMSCRIPTEN_CMAKE) \
	  -B$(WORKAREA)/$(CMAKE_BUILD_DIR)

# Configure the wasm tree from scratch.
.PHONY: configure
configure:
	$(RUN) rm -rf $(WORKAREA)/$(CMAKE_BUILD_DIR)
	$(MAKE) configure-wasm

.PHONY: build
build: configure-wasm
	$(RUN) cmake --build $(WORKAREA)/$(CMAKE_BUILD_DIR) --verbose

.PHONY: clean
clean:
	$(RUN) cmake --build $(WORKAREA)/$(CMAKE_BUILD_DIR) --verbose --target clean

# Back to a fresh checkout: every build tree, dist/ and the tool caches that
# .gitignore lists (python, clangd). Per-user files (.vscode/settings.json,
# *.code-workspace, .devcontainer/.env) are left alone.
# The IDE's CMake Tools names its trees after the kit (cmake-bld-<kit>.local,
# e.g. "cmake-bld-GCC 13.3.0 x86_64.local" after a kit scan), so glob rather
# than list the two trees make creates. The glob is expanded by the shell that
# runs the rm: in the container the paths exist, and glob matches stay single
# words even when the kit name has spaces.
DISTCLEAN_DIRS:=cmake-bld-*.local cmake-build-* build out dist \
  .mypy_cache .ruff_cache .pytest_cache .cache .clangd
DISTCLEAN_FILES:=compile_commands.json CMakeUserPresets.json

.PHONY: distclean
distclean:
	$(RUN) sh -c 'cd $(WORKAREA) && rm -rf $(DISTCLEAN_DIRS) $(DISTCLEAN_FILES) \
	  && find . -path ./.git -prune -o \( -name __pycache__ -o -name "*.py[cod]" \) -print0 \
	     | xargs -0 rm -rf'

# ---- native ----------------------------------------------------------------

.PHONY: configure-native
configure-native:
	$(RUN) $(CMAKE_CONFIGURE) -B$(WORKAREA)/$(NATIVE_BUILD_DIR)

.PHONY: native
native: configure-native
	$(RUN) cmake --build $(WORKAREA)/$(NATIVE_BUILD_DIR) --verbose

# Unit tests of the game logic (tests/), built and run from the native tree.
.PHONY: test
test: native
	$(RUN) ctest --test-dir $(WORKAREA)/$(NATIVE_BUILD_DIR) --output-on-failure --timeout 60

.PHONY: run
run: native
	$(RUN_X11) $(WORKAREA)/$(NATIVE_BUILD_DIR)/wasm-tic-tac-toe

# One-time macOS setup: let XQuartz accept TCP clients (same as ticking
# "Allow connections from network clients" in XQuartz > Settings > Security).
.PHONY: xquartz-tcp
xquartz-tcp:
ifeq ($(HOST_OS),Darwin)
	defaults write org.xquartz.X11 nolisten_tcp -bool false
	@echo "Restart XQuartz for this to take effect."
else
	$(error xquartz-tcp is macOS only; on $(HOST_OS) 'make run' uses the host X socket directly)
endif

.PHONY: server
server:
	$(SERVE) python3 -m http.server $(HTTP_PORT) -d $(WORKAREA)/dist

# ---- embedded resources ----------------------------------------------------

# Regenerate the resource headers (<tree>/generated/resources/*.h, made from
# resources/ by scripts/convert.py) in every configured build tree. The build
# does this on its own when a resource changes; this forces it.
.PHONY: includes
includes: configure-wasm
	$(RUN) sh -c 'for tree in $(CMAKE_BUILD_DIR) $(NATIVE_BUILD_DIR); do \
	  if [ -f $(WORKAREA)/$$tree/build.ninja ]; then \
	    rm -rf $(WORKAREA)/$$tree/generated; \
	    cmake --build $(WORKAREA)/$$tree --target includes; \
	  fi; \
	done'

# ---- code quality ----------------------------------------------------------

.PHONY: format
format:
	$(RUN) clang-format -i $(SOURCES)

# Same checks CI runs in its lint job.
.PHONY: lint
lint:
	$(RUN) clang-format --dry-run -Werror $(SOURCES)
	$(RUN) shellcheck $(SHELL_SCRIPTS)
	$(RUN) python3 -m py_compile scripts/convert.py

# clang-tidy runs against the native configure: the host clang-tidy can't parse
# emcc-only flags (-sUSE_SDL=2) in the wasm compile database. The generated
# resource headers must exist for the front end to parse. --quiet drops the
# "Suppressed N warnings" tally at the end; every finding is still printed.
.PHONY: tidy
tidy: configure-native
	$(RUN) cmake --build $(WORKAREA)/$(NATIVE_BUILD_DIR) --target includes
	$(RUN) clang-tidy --quiet -p $(WORKAREA)/$(NATIVE_BUILD_DIR) $(filter %.cpp,$(SOURCES))
