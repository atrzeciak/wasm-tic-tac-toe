# WebAssembly Tic-Tac-Toe

A Tic-Tac-Toe game written in C++ with SDL2, compiled to WebAssembly with
Emscripten. The same source also builds as a native Linux binary for fast
edit/debug cycles under gdb.

Play it live: [atrzeciak.github.io/wasm-tic-tac-toe](https://atrzeciak.github.io/wasm-tic-tac-toe)

## Prerequisites

Docker (Docker Desktop on macOS/Windows, or the engine on Linux). The whole
toolchain lives in the devcontainer image, which the first build creates:
Emscripten (pinned in `.devcontainer/Dockerfile`), SDL2/SDL2_ttf, Ninja,
clang-format, clang-tidy and shellcheck.

## Build

Every target runs inside the container. Run `make` from the host and it
wraps the command in `docker compose run`; run it from a devcontainer shell
(VS Code) and it runs directly.

```bash
make build     # wasm: writes dist/index.html, index.js, index.wasm
make native    # Linux binary: cmake-bld-native.local/wasm-tic-tac-toe
make server    # serve dist/ on http://localhost:8000
```

Other targets:

```bash
make image     # rebuild the devcontainer image (after editing the Dockerfile)
make lint      # clang-format --dry-run + shellcheck (same as CI)
make tidy      # clang-tidy against the native compile database
make format    # clang-format in place
make distclean # remove both build trees and dist/
```

Pass `BUILD_TYPE=Debug` to keep DWARF in the `.wasm` and enable
Emscripten's `SAFE_HEAP` and `ASSERTIONS`.

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

CI (`.github/workflows/ci.yml`) lints and builds on every PR and pushes
`dist/` to GitHub Pages on every push to `main`. Nothing is published by
hand; `dist/` is not tracked.

## Resources

`resources/convert.sh` regenerates the embedded image and font headers
(`*_bmp.h`, `*_ttf.h`) from the PNG and TTF sources.

## License

[MIT](LICENSE).
