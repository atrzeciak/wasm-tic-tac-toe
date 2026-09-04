# WebAssembly Tic-Tac-Toe

A Tic-Tac-Toe game written in C++ with SDL2, compiled to WebAssembly with
Emscripten. The same source also builds as a native Linux binary for fast
edit/debug cycles under gdb, and the game logic has unit tests.

Play it live: [atrzeciak.github.io/wasm-tic-tac-toe](https://atrzeciak.github.io/wasm-tic-tac-toe)

Space starts a game, click a cell to play. On the start screen `p`/`c` picks
who moves first and `h`/`e` the level; during a game `n` starts over and `q`
returns to the start screen. Game messages go to the console.

## Layout

```
src/main.cpp               SDL front end: window, input, rendering
src/game.h, game.cpp       game flow: settings, turns, end of game (no SDL)
src/board.h, board.cpp     rules and board state (no SDL)
src/ai.h, ai.cpp           computer opponent (no SDL)
tests/                     unit tests for the SDL-free parts, run by ctest
resources/                 PNGs and the font, embedded into the binary
scripts/convert.py         binary file -> C header, run by the build
scripts/convert.sh         regenerates every resource header by hand
scripts/install-emscripten.sh  emsdk install for the image and CI
web/shell.html             page template emcc wraps around the canvas
```

The build turns each file in `resources/` into a header holding its bytes
(`RedX.png` -> `RedX_png.h`) under `<build tree>/generated/resources/`.
Those headers are never committed; `make includes` regenerates them.

## Prerequisites

Docker (Docker Desktop on macOS/Windows, or the engine on Linux). The whole
toolchain lives in the devcontainer image, which the first build creates:
Emscripten (pinned in `.devcontainer/Dockerfile`), SDL2/SDL2_ttf/SDL2_image,
Ninja, clang-format, clang-tidy and shellcheck.

## Build

Every target runs inside the container. Run `make` from the host and it
wraps the command in `docker compose run`; run it from a devcontainer shell
(VS Code) and it runs directly.

```bash
make build     # wasm: writes dist/index.html, index.js, index.wasm
make native    # Linux binary: cmake-bld-native.local/wasm-tic-tac-toe
make test      # build the native tree and run the unit tests
make server    # serve dist/ on http://localhost:8000
make run       # run the native binary in the container, window on the host
```

`make run` needs an X server on the host. On macOS install XQuartz, run
`make xquartz-tcp` once and restart XQuartz; the container then connects
to it over TCP as `host.docker.internal:0`. On Linux the host's X socket is
mounted into the container and your `$DISPLAY` is used. Override either
with `X11_DISPLAY=`.

Other targets:

```bash
make image     # rebuild the devcontainer image (after editing the Dockerfile)
make lint      # clang-format --dry-run, shellcheck, py_compile (same as CI)
make tidy      # clang-tidy against the native compile database (same as CI)
make format    # clang-format in place
make includes  # regenerate the embedded-resource headers in every build tree
make configure # configure the wasm tree from scratch
make clean     # clean the wasm tree
make distclean # remove every build tree, dist/ and tool caches (fresh checkout)
```

Pass `BUILD_TYPE=Debug` to keep DWARF in the `.wasm` and enable
Emscripten's `SAFE_HEAP` and `ASSERTIONS`. Every build re-runs the
configure step, so switching `BUILD_TYPE` takes effect immediately.

## VS Code

Open the folder and choose "Reopen in Container". Two CMake Tools kits are
configured:

- **Emscripten**: builds into `cmake-bld-Emscripten.local`, same tree as
  `make build`. Debug with F5 "Debug wasm in Chrome"; breakpoints in the
  `.cpp` work through the WebAssembly DWARF Debugging extension.
- **native**: builds into `cmake-bld-native.local`, same tree as
  `make native`. The status-bar Run/Debug buttons launch it under gdb with
  the window on the forwarded X display.

## Deployment

CI (`.github/workflows/ci.yml`) lints, runs clang-tidy, builds the native
binary and its tests, and builds the site on every PR; every push to `main`
publishes `dist/` to GitHub Pages. Nothing is published by hand; `dist/` is
not tracked.

## License

[MIT](LICENSE).
