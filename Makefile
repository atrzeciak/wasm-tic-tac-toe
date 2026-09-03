
BUILD_TYPE:=Release
# Build trees are named cmake-bld-<kit>.local so they line up with the VS Code
# CMake Tools kits in .vscode/cmake-kits.json (see cmake.buildDirectory).
CMAKE_BUILD_DIR:=cmake-bld-Emscripten.local
# Native (non-wasm) tree: `make native` builds a Linux binary here, clang-tidy
# reads its compile database, and the IDE's "native" kit debugs it with gdb.
NATIVE_BUILD_DIR:=cmake-bld-native.local

WORKAREA:=/workarea
HTTP_PORT?=8000

# emsdk is baked into the image by the Dockerfile.
EMSDK_DIR?=/opt/emsdk
EMSCRIPTEN_CMAKE?="$(EMSDK_DIR)/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake"

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
else
# Pin the compose platform to the engine's arch (writes .devcontainer/.env).
$(if $(shell sh .devcontainer/host-platform.sh),,$(error host-platform.sh failed))
COMPOSE:=docker compose -f .devcontainer/docker-compose.yml
SERVICE:=wasm-tic-tac-toe
RUN:=$(COMPOSE) run --rm $(SERVICE)
SERVE:=$(COMPOSE) run --rm -p $(HTTP_PORT):$(HTTP_PORT) $(SERVICE)
endif

.PHONY: all
all:

.PHONY: image
image:
ifeq ($(IN_CONTAINER),1)
	@echo "Already inside the devcontainer; run 'make image' on the host to rebuild the image."
else
	$(COMPOSE) build
endif

$(CMAKE_BUILD_DIR):
	$(RUN) cmake \
	  -DCMAKE_TOOLCHAIN_FILE=$(EMSCRIPTEN_CMAKE) \
	  -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE \
	  -DCMAKE_BUILD_TYPE:STRING=$(BUILD_TYPE) \
	  -S$(WORKAREA) -B$(WORKAREA)/$(CMAKE_BUILD_DIR) -G Ninja

.PHONY: configure
configure:
	$(RUN) rm -rf $(WORKAREA)/$(CMAKE_BUILD_DIR)
	$(MAKE) $(CMAKE_BUILD_DIR)

.PHONY: build
build: $(CMAKE_BUILD_DIR)
	$(RUN) cmake --build $(WORKAREA)/$(CMAKE_BUILD_DIR) --config $(BUILD_TYPE) --target all --verbose

.PHONY: clean
clean:
	$(RUN) cmake --build $(WORKAREA)/$(CMAKE_BUILD_DIR) --verbose --target clean

.PHONY: distclean
distclean:
	$(RUN) rm -rf $(WORKAREA)/$(CMAKE_BUILD_DIR) $(WORKAREA)/$(NATIVE_BUILD_DIR) $(WORKAREA)/dist

.PHONY: format
format:
	$(RUN) clang-format -i wasm-tic-tac-toe.cpp

# Same checks CI runs in its lint job.
.PHONY: lint
lint:
	$(RUN) clang-format --dry-run -Werror wasm-tic-tac-toe.cpp
	$(RUN) shellcheck install-emscripten.sh resources/convert.sh .devcontainer/host-platform.sh

# clang-tidy runs against a native configure: the host clang-tidy can't parse
# emcc-only flags (-sUSE_SDL=2) in the wasm compile database.
$(NATIVE_BUILD_DIR):
	$(RUN) cmake \
	  -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE \
	  -DCMAKE_BUILD_TYPE:STRING=$(BUILD_TYPE) \
	  -S$(WORKAREA) -B$(WORKAREA)/$(NATIVE_BUILD_DIR) -G Ninja

.PHONY: tidy
tidy: $(NATIVE_BUILD_DIR)
	$(RUN) clang-tidy -p $(WORKAREA)/$(NATIVE_BUILD_DIR) wasm-tic-tac-toe.cpp

.PHONY: native
native: $(NATIVE_BUILD_DIR)
	$(RUN) cmake --build $(WORKAREA)/$(NATIVE_BUILD_DIR) --config $(BUILD_TYPE) --target all --verbose

.PHONY: server
server:
	$(SERVE) python3 -m http.server $(HTTP_PORT) -d $(WORKAREA)/dist
