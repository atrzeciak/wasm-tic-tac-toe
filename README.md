
# WebAssembly Tic-Tac-Toe

A **Tic-Tac-Toe game** implemented in **C++** and compiled to **WebAssembly (WASM)**. This project showcases how C++ can be used to create interactive browser-based games with WebAssembly for fast, efficient performance.

## Features

- **High Performance:** Core logic implemented in C++ and compiled to WebAssembly for near-native execution speed.
- **Simple Gameplay:** A lightweight implementation of the classic Tic-Tac-Toe game.
- **Cross-Browser Support:** Runs in any WebAssembly-compatible browser.

## Demo

Play the game live: [Tic-Tac-Toe WebAssembly Demo](https://atrzeciak.github.io/wasm-tic-tac-toe)

## Getting Started

### Prerequisites

To build and run this project locally, you will need:

1. **Emscripten SDK** for compiling C++ to WebAssembly:
   - Install the Emscripten SDK

      ```bash
      bash ./install-emscripten.sh
      ```

2. A web server to serve the compiled files (e.g., Python's HTTP server or Node.js-based servers).

### InstallationCompile the project using Emscripten

```bash
# Add emscripten tools to your path
source emsdk/emsdk_env.sh

# cmake and build
mkdir build
emcmake cmake -DCMAKE_BUILD_TYPE=Release -S. -B build -G Ninja
cmake --build build --target all --verbose
```

### Serve the project files

```bash
python3 -m http.server -d dist/
```

### Open a browser, and navigate to [http://localhost:8000](http://localhost:8000)

### Usage

- Launch the game in your browser.
- Play against a computer simple logic.

## License

This project is licensed under the [MIT License](LICENSE).

## Acknowledgments

- Built using [Emscripten](https://emscripten.org/) to compile C++ into WebAssembly.
