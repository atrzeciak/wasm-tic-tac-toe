// SDL front end: window, input translation and rendering. The game's flow is
// in game.h, the rules in board.h and the opponent in ai.h; all three are
// plain C++ and unit tested.
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <utility>

#include "board.h"
#include "game.h"
#include "resources/RedO_png.h"
#include "resources/RedX_png.h"
#include "resources/RobotoMono_Regular_ttf.h"

namespace {

using ttt::Line;
using ttt::Mark;

constexpr int kInitialSize = 720;
constexpr int kFontPt = 36;
constexpr int kCellInset = 10;
constexpr int kLineHalfWidth = 1;  // winner line is 2 * this + 1 pixels wide
#ifndef __EMSCRIPTEN__
constexpr Uint32 kFrameDelayMs = 16;  // the browser paces the wasm loop itself
#endif

constexpr SDL_Color kWhite{255, 255, 255, 255};
constexpr SDL_Color kGreen{32, 255, 0, 255};
constexpr SDL_Color kMagenta{255, 0, 255, 255};
constexpr SDL_Color kBlue{0, 0, 255, 255};
constexpr SDL_Color kBorder{64, 64, 64, 255};
constexpr SDL_Color kCellBackground{32, 32, 32, 255};
constexpr SDL_Color kBlack{0, 0, 0, 255};

const char *verdictText(Mark winner) {
  if (winner == Mark::X) {
    return "X Won";
  }
  if (winner == Mark::O) {
    return "O Won";
  }
  return "Draw";
}

std::uint32_t pack(SDL_Color c) {
  return (static_cast<std::uint32_t>(c.r) << 24U) |
         (static_cast<std::uint32_t>(c.g) << 16U) |
         (static_cast<std::uint32_t>(c.b) << 8U) | c.a;
}

class App {
 public:
  App() = default;
  ~App() { finalize(); }
  App(const App &) = delete;
  App &operator=(const App &) = delete;
  App(App &&) = delete;
  App &operator=(App &&) = delete;

  // Creates the window and loads the embedded resources. False, with the
  // reason on stderr, if SDL refuses; the app must not be run then.
  bool initialize();
  void finalize();

  // Runs one frame: input, the computer's move, rendering. False once the
  // user closes the window or presses Escape.
  bool frame();

 private:
  bool loadResources();
  SDL_Texture *loadPng(const unsigned char *bytes, int size);

  bool handleEvents();
  void keyPressed(SDL_Keycode sym);
  void mousePressed(int mx, int my);
  void computerTurn();

  void resize();
  [[nodiscard]] SDL_Rect cellRect(int idx) const;
  void renderSplash();
  void renderBoard();
  void renderResult();
  void drawWinnerLine(const Line &line);
  void setColor(SDL_Color c);
  // Draws `str` centred on x with its top edge at y.
  void text(const std::string &str, SDL_Color color, int x, int y);
  SDL_Texture *textTexture(const std::string &str, SDL_Color color);

  int d_width = kInitialSize;
  int d_height = kInitialSize;
  SDL_Window *d_window = nullptr;
  SDL_Renderer *d_renderer = nullptr;
  SDL_Texture *d_xTexture = nullptr;
  SDL_Texture *d_oTexture = nullptr;
  TTF_Font *d_font = nullptr;
  // Rendered labels, keyed by text and colour, so a frame never re-rasterises.
  std::map<std::pair<std::string, std::uint32_t>, SDL_Texture *> d_textCache;

  ttt::Game d_game;
};

bool App::initialize() {
#ifndef __EMSCRIPTEN__
  // We render with SDL_RENDERER_SOFTWARE, but SDL still opens a GLX context
  // to probe for accelerated framebuffer blits. Inside the devcontainer the
  // forwarded X display has no usable GLX, which aborts the process; skip it.
  ::SDL_SetHint(SDL_HINT_FRAMEBUFFER_ACCELERATION, "0");
#endif
  if (::SDL_Init(SDL_INIT_VIDEO) != 0) {
    std::cerr << "SDL_Init: " << ::SDL_GetError() << '\n';
    return false;
  }
  if (::TTF_Init() != 0) {
    std::cerr << "TTF_Init: " << ::TTF_GetError() << '\n';
    return false;
  }
  if ((::IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) == 0) {
    std::cerr << "IMG_Init: " << ::IMG_GetError() << '\n';
    return false;
  }

  Uint32 flags = 0;
#ifndef __EMSCRIPTEN__
  // In the browser SDL would size a resizable canvas to the whole page; the
  // shell's CSS scales the fixed canvas instead.
  flags |= SDL_WINDOW_RESIZABLE;
#endif
  d_window =
      ::SDL_CreateWindow("wasm-tic-tac-toe", SDL_WINDOWPOS_UNDEFINED,
                         SDL_WINDOWPOS_UNDEFINED, d_width, d_height, flags);
  if (d_window == nullptr) {
    std::cerr << "SDL_CreateWindow: " << ::SDL_GetError() << '\n';
    return false;
  }
  d_renderer = ::SDL_CreateRenderer(d_window, -1, SDL_RENDERER_SOFTWARE);
  if (d_renderer == nullptr) {
    std::cerr << "SDL_CreateRenderer: " << ::SDL_GetError() << '\n';
    return false;
  }
  return loadResources();
}

bool App::loadResources() {
  d_font = ::TTF_OpenFontRW(
      ::SDL_RWFromConstMem(RobotoMono_Regular_ttf,
                           static_cast<int>(sizeof(RobotoMono_Regular_ttf))),
      1, kFontPt);
  if (d_font == nullptr) {
    std::cerr << "TTF_OpenFontRW: " << ::TTF_GetError() << '\n';
    return false;
  }
  d_xTexture = loadPng(RedX_png, static_cast<int>(sizeof(RedX_png)));
  d_oTexture = loadPng(RedO_png, static_cast<int>(sizeof(RedO_png)));
  return d_xTexture != nullptr && d_oTexture != nullptr;
}

SDL_Texture *App::loadPng(const unsigned char *bytes, int size) {
  SDL_Surface *surface = ::IMG_Load_RW(::SDL_RWFromConstMem(bytes, size), 1);
  if (surface == nullptr) {
    std::cerr << "IMG_Load_RW: " << ::IMG_GetError() << '\n';
    return nullptr;
  }
  SDL_Texture *texture = ::SDL_CreateTextureFromSurface(d_renderer, surface);
  ::SDL_FreeSurface(surface);
  if (texture == nullptr) {
    std::cerr << "SDL_CreateTextureFromSurface: " << ::SDL_GetError() << '\n';
  }
  return texture;
}

void App::finalize() {
  for (auto &entry : d_textCache) {
    ::SDL_DestroyTexture(entry.second);
  }
  d_textCache.clear();
  if (d_xTexture != nullptr) {
    ::SDL_DestroyTexture(d_xTexture);
    d_xTexture = nullptr;
  }
  if (d_oTexture != nullptr) {
    ::SDL_DestroyTexture(d_oTexture);
    d_oTexture = nullptr;
  }
  if (d_font != nullptr) {
    ::TTF_CloseFont(d_font);
    d_font = nullptr;
  }
  if (d_renderer != nullptr) {
    ::SDL_DestroyRenderer(d_renderer);
    d_renderer = nullptr;
  }
  if (d_window != nullptr) {
    ::SDL_DestroyWindow(d_window);
    d_window = nullptr;
  }
  ::IMG_Quit();
  ::TTF_Quit();
  ::SDL_Quit();
}

// ---- input ------------------------------------------------------------------

bool App::handleEvents() {
  SDL_Event event;
  while (::SDL_PollEvent(&event) != 0) {
    switch (event.type) {
      case SDL_QUIT:
        return false;
      case SDL_KEYDOWN:
        if (event.key.keysym.sym == SDLK_ESCAPE) {
          return false;
        }
        keyPressed(event.key.keysym.sym);
        break;
      case SDL_MOUSEBUTTONDOWN:
        mousePressed(event.button.x, event.button.y);
        break;
      default:
        break;
    }
  }
  return true;
}

// Start screen: Space plays, c/p picks who opens, h/e the level.
// In a game: n restarts, q returns to the start screen.
void App::keyPressed(SDL_Keycode sym) {
  if (d_game.playing()) {
    if (sym == SDLK_n) {
      std::cout << "New game\n";
      d_game.start();
    } else if (sym == SDLK_q) {
      std::cout << "Quit game\n";
      d_game.quit();
    }
    return;
  }
  switch (sym) {
    case SDLK_SPACE:
      std::cout << "Start of game\n";
      d_game.start();
      break;
    case SDLK_c:
      std::cout << "Computer first\n";
      d_game.setComputerFirst(true);
      break;
    case SDLK_p:
      std::cout << "Human first\n";
      d_game.setComputerFirst(false);
      break;
    case SDLK_h:
      std::cout << "Hard level\n";
      d_game.setHard(true);
      break;
    case SDLK_e:
      std::cout << "Easy level\n";
      d_game.setHard(false);
      break;
    default:
      break;
  }
}

void App::mousePressed(int mx, int my) {
  const SDL_Point point{mx, my};
  for (int idx = 0; idx < ttt::kCells; ++idx) {
    const SDL_Rect rect = cellRect(idx);
    if (::SDL_PointInRect(&point, &rect) == SDL_TRUE) {
      d_game.humanPlays(idx);  // ignored unless it is a legal human move
      return;
    }
  }
}

void App::computerTurn() {
  if (const char *reason = d_game.computerTurn()) {
    std::cout << reason << '\n';
    if (d_game.finished()) {
      std::cout << verdictText(d_game.result().winner) << '\n';
    }
  }
}

// ---- rendering --------------------------------------------------------------

void App::resize() {
  int w = 0;
  int h = 0;
  ::SDL_GetRendererOutputSize(d_renderer, &w, &h);
  if (w != d_width || h != d_height) {
    std::cout << "resize " << w << 'x' << h << '\n';
    d_width = w;
    d_height = h;
  }
}

SDL_Rect App::cellRect(int idx) const {
  const int w = d_width / ttt::kSize;
  const int h = d_height / ttt::kSize;
  return SDL_Rect{ttt::cellCol(idx) * w, ttt::cellRow(idx) * h, w, h};
}

void App::setColor(SDL_Color c) {
  ::SDL_SetRenderDrawColor(d_renderer, c.r, c.g, c.b, c.a);
}

bool App::frame() {
  if (!handleEvents()) {
    return false;
  }
  resize();

  setColor(kBlack);
  ::SDL_RenderClear(d_renderer);
  setColor(kBorder);
  ::SDL_RenderDrawRect(d_renderer, nullptr);

  if (d_game.playing()) {
    computerTurn();
    renderBoard();
    renderResult();
  } else {
    renderSplash();
  }
  ::SDL_RenderPresent(d_renderer);
  return true;
}

void App::renderSplash() {
  const int x = d_width / 2;
  const int row = d_height / 8;
  text("Welcome to Tic Tac Toe", kWhite, x, row);
  text(d_game.computerFirst() ? "Computer first [p,c]" : "Human first [p,c]",
       kGreen, x, row * 3);
  text(d_game.hard() ? "Hard Level [h,e]" : "Easy Level [h,e]", kMagenta, x,
       row * 4);
  text("Press the SpaceBar to Play", kWhite, x, row * 7);
}

void App::renderBoard() {
  for (int idx = 0; idx < ttt::kCells; ++idx) {
    SDL_Rect rect = cellRect(idx);
    rect.x += 1;
    rect.y += 1;
    rect.w -= 2;
    rect.h -= 2;
    setColor(kCellBackground);
    ::SDL_RenderFillRect(d_renderer, &rect);

    rect.x += kCellInset;
    rect.y += kCellInset;
    rect.w -= 2 * kCellInset;
    rect.h -= 2 * kCellInset;
    const Mark m = d_game.board().at(idx);
    if (m == Mark::X) {
      ::SDL_RenderCopy(d_renderer, d_xTexture, nullptr, &rect);
    } else if (m == Mark::O) {
      ::SDL_RenderCopy(d_renderer, d_oTexture, nullptr, &rect);
    }
  }
}

void App::renderResult() {
  const ttt::Result &result = d_game.result();
  if (!result.finished) {
    return;
  }
  if (result.line) {
    drawWinnerLine(*result.line);
  }
  text(verdictText(result.winner), kWhite, d_width / 2, d_height / 2);
  text("New Game [n] Quit [q]", kGreen, d_width / 2, d_height / 4);
}

// A thick line through the centres of the first and last cell of `line`.
void App::drawWinnerLine(const Line &line) {
  const SDL_Rect from = cellRect(line.front());
  const SDL_Rect to = cellRect(line.back());
  const int x1 = from.x + from.w / 2;
  const int y1 = from.y + from.h / 2;
  const int x2 = to.x + to.w / 2;
  const int y2 = to.y + to.h / 2;
  setColor(kBlue);
  for (int d = -kLineHalfWidth; d <= kLineHalfWidth; ++d) {
    ::SDL_RenderDrawLine(d_renderer, x1 + d, y1 + d, x2 + d, y2 + d);
  }
}

void App::text(const std::string &str, SDL_Color color, int x, int y) {
  SDL_Texture *texture = textTexture(str, color);
  if (texture == nullptr) {
    return;
  }
  SDL_Rect dest{0, y, 0, 0};
  ::SDL_QueryTexture(texture, nullptr, nullptr, &dest.w, &dest.h);
  dest.x = x - dest.w / 2;
  ::SDL_RenderCopy(d_renderer, texture, nullptr, &dest);
}

SDL_Texture *App::textTexture(const std::string &str, SDL_Color color) {
  const auto key = std::make_pair(str, pack(color));
  const auto found = d_textCache.find(key);
  if (found != d_textCache.end()) {
    return found->second;
  }
  SDL_Surface *surface = ::TTF_RenderText_Blended(d_font, str.c_str(), color);
  if (surface == nullptr) {
    std::cerr << "TTF_RenderText_Blended: " << ::TTF_GetError() << '\n';
    return nullptr;
  }
  SDL_Texture *texture = ::SDL_CreateTextureFromSurface(d_renderer, surface);
  ::SDL_FreeSurface(surface);
  d_textCache.emplace(key, texture);
  return texture;
}

#ifdef __EMSCRIPTEN__
void mainLoop(void *userData) {
  // Escape does nothing in the browser: there is no window to close, and a
  // stopped main loop would just leave a frozen canvas.
  static_cast<App *>(userData)->frame();
}
#endif

}  // namespace

int main() {
  App app;
  if (!app.initialize()) {
    return 1;
  }

#ifdef __EMSCRIPTEN__
  // simulate_infinite_loop=1: this call never returns, and `app` stays valid
  // because Emscripten keeps main's frame alive.
  emscripten_set_main_loop_arg(mainLoop, &app, 0, 1);
#else
  while (app.frame()) {
    ::SDL_Delay(kFrameDelayMs);
  }
#endif
  return 0;
}
