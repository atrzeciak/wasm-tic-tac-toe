#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#else
#include <SDL.h>
#include <SDL_ttf.h>
#endif

#include "resources/RedO_bmp.h"
#include "resources/RedX_bmp.h"
#include "resources/RobotoMono_Regular_ttf.h"

#include <iostream>
#include <vector>

namespace {
const int NUM_COLS = 3;
const int NUM_ROWS = 3;

enum CellState { CELL_EMPTY = ' ', CELL_X = 'X', CELL_O = 'O' };

CellState switchXO(CellState c) { return c == CELL_X ? CELL_O : CELL_X; }

struct Cell {
  int x;
  int y;
  int w;
  int h;
  CellState state;

  Cell() : x(0), y(0), w(0), h(0), state(CELL_EMPTY) {}

  void resize(int tx, int ty, int tw, int th) {
    x = tx;
    y = ty;
    w = tw;
    h = th;
  }

  bool inRange(int mx, int my) {
    return mx > x && mx < x + w && my > y && my < y + h;
  }

  void reset() { state = CELL_EMPTY; }
};

class TicTacToe {
 private:
  enum WinnerLine {
    NONE,
    VERTICAL_LEFT,
    VERTICAL_MID,
    VERTICAL_RIGHT,
    HORIZONTAL_TOP,
    HORIZONTAL_MID,
    HORIZONTAL_BOTTOM,
    DIAGONAL_TOPLEFT,
    DIAGONAL_BOTTOMLEFT
  };

  int d_display_width;
  int d_display_height;

  ::SDL_Window *d_window;
  ::SDL_Renderer *d_renderer;

  ::SDL_Surface *d_RedX_Surface;
  ::SDL_Texture *d_RedX_Texture;

  ::SDL_Surface *d_RedO_Surface;
  ::SDL_Texture *d_RedO_Texture;

  ::TTF_Font *d_ttf_font;

  ::SDL_Color d_textColor;
  ::SDL_Color d_background;

  Cell d_board[3][3];

  CellState d_firstMove;
  CellState d_computerPlays;
  bool d_hardLevel;
  CellState d_currentPlayer;
  bool d_gameInProgress;
  bool d_gameFinished;
  CellState d_gameWinner;
  WinnerLine d_winnerLine;

 public:
  TicTacToe()
      : d_display_width(720),
        d_display_height(720),
        d_window(0),
        d_renderer(0),
        d_RedX_Surface(0),
        d_RedX_Texture(0),
        d_RedO_Surface(0),
        d_RedO_Texture(0),
        d_ttf_font(0),
        d_textColor(),
        d_background(),
        d_board(),
        d_firstMove(CELL_O),
        d_computerPlays(CELL_O),
        d_hardLevel(true),
        d_currentPlayer(CELL_EMPTY),
        d_gameInProgress(false),
        d_gameFinished(false),
        d_winnerLine(NONE) {
    textColor();
    background();
    setBoardSize(d_display_width, d_display_height);

    initGame();
  }

  ~TicTacToe() { ::SDL_Quit(); }

  WinnerLine getWinnerLine(CellState s) {
    if ((cellState(0, 0) == s && cellState(0, 1) == s &&
         cellState(0, 2) == s)) {
      return VERTICAL_LEFT;
    }
    if ((cellState(1, 2) == s && cellState(1, 1) == s &&
         cellState(1, 0) == s)) {
      return VERTICAL_MID;
    }
    if ((cellState(2, 2) == s && cellState(2, 1) == s &&
         cellState(2, 0) == s)) {
      return VERTICAL_RIGHT;
    }
    if ((cellState(0, 0) == s && cellState(1, 0) == s &&
         cellState(2, 0) == s)) {
      return HORIZONTAL_TOP;
    }
    if ((cellState(0, 1) == s && cellState(1, 1) == s &&
         cellState(2, 1) == s)) {
      return HORIZONTAL_MID;
    }
    if ((cellState(0, 2) == s && cellState(1, 2) == s &&
         cellState(2, 2) == s)) {
      return HORIZONTAL_BOTTOM;
    }
    if ((cellState(0, 0) == s && cellState(1, 1) == s &&
         cellState(2, 2) == s)) {
      return DIAGONAL_TOPLEFT;
    }
    if ((cellState(0, 2) == s && cellState(1, 1) == s &&
         cellState(2, 0) == s)) {
      return DIAGONAL_BOTTOMLEFT;
    }
    return NONE;
  }

  bool isBoardFull() {
    for (int col = 0; col < NUM_COLS; col++) {
      for (int row = 0; row < NUM_ROWS; row++) {
        if (cellState(col, row) == CELL_EMPTY) {
          return false;
        }
      }
    }
    return true;
  }

  void checkEndofGame() {
    if (!d_gameFinished) {
      WinnerLine winnerLine = getWinnerLine(CELL_X);
      if (NONE != winnerLine) {
        std::cout << "Winner X" << std::endl;
        d_gameWinner = CELL_X;
        d_winnerLine = winnerLine;
        d_gameFinished = true;
      } else {
        WinnerLine winnerLine = getWinnerLine(CELL_O);
        if (NONE != winnerLine) {
          std::cout << "Winner O" << std::endl;
          d_gameWinner = CELL_O;
          d_winnerLine = winnerLine;
          d_gameFinished = true;
        } else if (isBoardFull()) {
          std::cout << "No more moves" << std::endl;
          d_gameWinner = CELL_EMPTY;
          d_winnerLine = NONE;
          d_gameFinished = true;
        }
      }
    }
  }

  void makeComputerMove() {
    if (d_gameInProgress && !d_gameFinished) {
      if (d_currentPlayer == d_computerPlays) {
        if (d_hardLevel) {
          advancedMove(d_computerPlays);
        } else {
          randomMove(d_computerPlays);
        }
        d_currentPlayer = switchXO(d_computerPlays);
      }
    }

    checkEndofGame();
  }

  void mousePressed(const SDL_Event &event) {
    const int mouseX = event.button.x;
    const int mouseY = event.button.y;
    if (d_gameInProgress && !d_gameFinished) {
      cellClick(mouseX, mouseY);
    }

    checkEndofGame();
  }

  void keyPressed(const SDL_Event &event) {
    const SDL_Keycode sym = event.key.keysym.sym;

    if (!d_gameInProgress) {
      if (sym == SDLK_SPACE) {
        std::cout << "Start of game" << std::endl;
        d_gameInProgress = true;
        d_gameFinished = false;
        initGame();
        return;
      }

      if (sym == SDLK_c) {
        if (d_firstMove != d_computerPlays) {
          std::cout << "Computer first" << std::endl;
        }
        d_firstMove = d_computerPlays;
        return;
      }

      if (sym == SDLK_p) {
        if (d_firstMove == d_computerPlays) {
          std::cout << "Human first" << std::endl;
        }
        d_firstMove = switchXO(d_computerPlays);
        return;
      }

      if (sym == SDLK_h) {
        if (!d_hardLevel) {
          std::cout << "Hard Level" << std::endl;
        }
        d_hardLevel = true;
        return;
      }

      if (sym == SDLK_e) {
        if (d_hardLevel) {
          std::cout << "Easy Level" << std::endl;
        }
        d_hardLevel = false;
        return;
      }
    } else {
      if (sym == SDLK_n) {
        std::cout << "New game" << std::endl;
        d_gameInProgress = true;
        d_gameFinished = false;
        initGame();
        return;
      }
      if (sym == SDLK_q) {
        std::cout << "Quit game" << std::endl;
        d_gameInProgress = false;
        d_gameFinished = false;
        initGame();
        return;
      }
    }
  }

  int boardIndex(int col, int row) { return col + NUM_ROWS * row; }

  void makeMove(int idx, CellState p) {
    // 0[0,0], 1[1,0], 2[2,0]
    // 3[0,1], 4[1,1], 5[2,1]
    // 6[0,2], 7[1,2], 8[2,2]
    switch (idx) {
      case 0:
        setCellState(0, 0, p);  // top left
        break;
      case 1:
        setCellState(1, 0, p);
        break;
      case 2:
        setCellState(2, 0, p);  // top right
        break;
      case 3:
        setCellState(0, 1, p);
        break;
      case 4:
        setCellState(1, 1, p);  // center
        break;
      case 5:
        setCellState(2, 1, p);
        break;
      case 6:
        setCellState(0, 2, p);  // bottom left
        break;
      case 7:
        setCellState(1, 2, p);
        break;
      case 8:
        setCellState(2, 2, p);  // bottom right
        break;
    }
  }

  void makeRandomMoveFromList(const std::vector<int> &a, CellState p) {
    const int r = rand() % a.size();
    makeMove(a[r], p);
  }

  bool pickCorner(const CellState &p) {
    // Try to take one of the corners, if they are free.
    // 0[0,0], 1[1,0], 2[2,0]
    // 3[0,1], 4[1,1], 5[2,1]
    // 6[0,2], 7[1,2], 8[2,2]
    std::vector<int> corners;
    if (cellState(0, 0) == CELL_EMPTY) {
      corners.push_back(boardIndex(0, 0));
    }
    if (cellState(2, 0) == CELL_EMPTY) {
      corners.push_back(boardIndex(2, 0));
    }
    if (cellState(0, 2) == CELL_EMPTY) {
      corners.push_back(boardIndex(0, 2));
    }
    if (cellState(2, 2) == CELL_EMPTY) {
      corners.push_back(boardIndex(2, 2));
    }
    if (corners.empty()) {
      return false;
    }
    makeRandomMoveFromList(corners, p);
    std::cout << "Take a corner" << std::endl;
    return true;
  }

  bool pickCenter(const CellState &p) {
    // Try to take the center, if it is free.
    if (cellState(1, 1) == CELL_EMPTY) {
      // make a move
      setCellState(1, 1, p);
      std::cout << "Take a center" << std::endl;
      return true;
    }
    return false;
  }

  bool pickSide(const CellState &p) {
    // Move on one of the sides.
    // 0[0,0], 1[1,0], 2[2,0]
    // 3[0,1], 4[1,1], 5[2,1]
    // 6[0,2], 7[1,2], 8[2,2]
    std::vector<int> sides;
    if (cellState(1, 0) == CELL_EMPTY) {
      sides.push_back(boardIndex(1, 0));
    }
    if (cellState(0, 1) == CELL_EMPTY) {
      sides.push_back(boardIndex(0, 1));
    }
    if (cellState(2, 1) == CELL_EMPTY) {
      sides.push_back(boardIndex(2, 1));
    }
    if (cellState(1, 2) == CELL_EMPTY) {
      sides.push_back(boardIndex(1, 2));
    }
    if (sides.empty()) {
      return false;
    }
    makeRandomMoveFromList(sides, p);
    std::cout << "Choose random side" << std::endl;
    return true;
  }

  void advancedMove(const CellState &p) {
    // First, check if we computer win in the next move
    for (int col = 0; col < NUM_COLS; col++) {
      for (int row = 0; row < NUM_ROWS; row++) {
        if (cellState(col, row) == CELL_EMPTY) {
          // make a move
          setCellState(col, row, p);
          if (NONE != getWinnerLine(p)) {
            std::cout << "Make a winning move" << std::endl;
            return;
          }
          // undo a move
          setCellState(col, row, CELL_EMPTY);
        }
      }
    }

    // Check if the player could win on their next move, and block them.
    const CellState z = switchXO(p);
    for (int col = 0; col < NUM_COLS; col++) {
      for (int row = 0; row < NUM_ROWS; row++) {
        if (cellState(col, row) == CELL_EMPTY) {
          // make a move
          setCellState(col, row, z);
          if (NONE != getWinnerLine(z)) {
            // undo a move
            setCellState(col, row, CELL_EMPTY);
            // make a move
            setCellState(col, row, p);
            std::cout << "Block a player" << std::endl;
            return;
          }
          // undo a move
          setCellState(col, row, CELL_EMPTY);
        }
      }
    }

    if (!pickCorner(p)) {
      if (!pickCenter(p)) {
        if (!pickSide(p)) {
          std::cout << "hmm, is the board full" << std::endl;
        }
      }
    }

    return;
  }

  void randomMove(CellState p) {
    // Make a random move
    std::vector<int> possibleMoves;
    for (int col = 0; col < NUM_COLS; col++) {
      for (int row = 0; row < NUM_ROWS; row++) {
        if (cellState(col, row) == CELL_EMPTY) {
          possibleMoves.push_back(boardIndex(col, row));
        }
      }
    }
    const int r = rand() % possibleMoves.size();
    makeMove(possibleMoves[r], p);
  }

  void setBoardSize(int w, int h) {
    int cell_width = w / NUM_COLS;
    int cell_height = h / NUM_ROWS;
    for (int col = 0; col < NUM_COLS; col++) {
      for (int row = 0; row < NUM_ROWS; row++) {
        Cell &cell = d_board[col][row];

        cell.resize(col * cell_width, row * cell_height, cell_width,
                    cell_height);
      }
    }
  }

  void setCellState(int col, int row, CellState p) {
    Cell &cell = d_board[col][row];

    cell.state = p;
  }

  int cellState(int col, int row) {
    const Cell &cell = d_board[col][row];

    return cell.state;
  }

  bool cellClick(int mx, int my) {
    for (int col = 0; col < NUM_COLS; col++) {
      for (int row = 0; row < NUM_ROWS; row++) {
        Cell &cell = d_board[col][row];

        if (cell.inRange(mx, my)) {
          if (cell.state == CELL_EMPTY) {
            cell.state = d_currentPlayer;
            d_currentPlayer = switchXO(d_currentPlayer);
            return true;
          }
        }
      }
    }
    return false;
  }

  void cellRender(int col, int row) {
    const Cell &cell = d_board[col][row];

    SDL_Rect rect;

    rect.x = cell.x + 1;
    rect.y = cell.y + 1;
    rect.w = cell.w - 2;
    rect.h = cell.h - 2;

    ::SDL_SetRenderDrawColor(d_renderer, 32, 32, 32, 255);
    ::SDL_RenderFillRect(d_renderer, &rect);

    rect.x += 10;
    rect.y += 10;
    rect.w -= 20;
    rect.h -= 20;

    if (cell.state == CELL_O) {
      ::SDL_RenderCopy(d_renderer, d_RedO_Texture, NULL, &rect);
    } else if (cell.state == CELL_X) {
      ::SDL_RenderCopy(d_renderer, d_RedX_Texture, NULL, &rect);
    }
  }

  void boardRender() {
    for (int col = 0; col < NUM_COLS; col++) {
      for (int row = 0; row < NUM_ROWS; row++) {
        cellRender(col, row);
      }
    }
  }

  void boardWinnerRender() {
    const int x_step = d_display_width / 6;
    const int y_step = d_display_height / 6;

    if (d_gameFinished) {
      SDL_Rect rect;
      switch (d_winnerLine) {
        case NONE:
          // tie
          break;
        case VERTICAL_LEFT:
          // draw line
          ::SDL_SetRenderDrawColor(d_renderer, 0, 0, 255, 255);
          ::SDL_RenderDrawLine(d_renderer, x_step - 1, y_step, x_step - 1,
                               y_step * 5);
          ::SDL_RenderDrawLine(d_renderer, x_step, y_step, x_step, y_step * 5);
          ::SDL_RenderDrawLine(d_renderer, x_step + 1, y_step, x_step + 1,
                               y_step * 5);
          break;
        case VERTICAL_MID:
          // draw line
          ::SDL_SetRenderDrawColor(d_renderer, 0, 0, 255, 255);
          ::SDL_RenderDrawLine(d_renderer, x_step * 3 - 1, y_step,
                               x_step * 3 - 1, y_step * 5);
          ::SDL_RenderDrawLine(d_renderer, x_step * 3, y_step, x_step * 3,
                               y_step * 5);
          ::SDL_RenderDrawLine(d_renderer, x_step * 3 + 1, y_step,
                               x_step * 3 + 1, y_step * 5);
          break;
        case VERTICAL_RIGHT:
          // draw line
          ::SDL_SetRenderDrawColor(d_renderer, 0, 0, 255, 255);
          ::SDL_RenderDrawLine(d_renderer, x_step * 5 - 1, y_step,
                               x_step * 5 - 1, y_step * 5);
          ::SDL_RenderDrawLine(d_renderer, x_step * 5, y_step, x_step * 5,
                               y_step * 5);
          ::SDL_RenderDrawLine(d_renderer, x_step * 5 + 1, y_step,
                               x_step * 5 + 1, y_step * 5);
          break;
        case HORIZONTAL_TOP:
          // draw line
          ::SDL_SetRenderDrawColor(d_renderer, 0, 0, 255, 255);
          ::SDL_RenderDrawLine(d_renderer, x_step, y_step - 1, x_step * 5,
                               y_step - 1);
          ::SDL_RenderDrawLine(d_renderer, x_step, y_step, x_step * 5, y_step);
          ::SDL_RenderDrawLine(d_renderer, x_step, y_step + 1, x_step * 5,
                               y_step + 1);
          break;
        case HORIZONTAL_MID:
          // draw line
          ::SDL_SetRenderDrawColor(d_renderer, 0, 0, 255, 255);
          ::SDL_RenderDrawLine(d_renderer, x_step, y_step * 3 - 1, x_step * 5,
                               y_step * 3 - 1);
          ::SDL_RenderDrawLine(d_renderer, x_step, y_step * 3, x_step * 5,
                               y_step * 3);
          ::SDL_RenderDrawLine(d_renderer, x_step, y_step * 3 + 1, x_step * 5,
                               y_step * 3 + 1);
          break;
        case HORIZONTAL_BOTTOM:
          // draw line
          ::SDL_SetRenderDrawColor(d_renderer, 0, 0, 255, 255);
          ::SDL_RenderDrawLine(d_renderer, x_step, y_step * 5 - 1, x_step * 5,
                               y_step * 5 - 1);
          ::SDL_RenderDrawLine(d_renderer, x_step, y_step * 5, x_step * 5,
                               y_step * 5);
          ::SDL_RenderDrawLine(d_renderer, x_step, y_step * 5 + 1, x_step * 5,
                               y_step * 5 + 1);
          break;
        case DIAGONAL_TOPLEFT:
          // draw line
          ::SDL_SetRenderDrawColor(d_renderer, 0, 0, 255, 255);
          ::SDL_RenderDrawLine(d_renderer, x_step - 1, y_step - 1,
                               x_step * 5 - 1, y_step * 5 - 1);
          ::SDL_RenderDrawLine(d_renderer, x_step, y_step, x_step * 5,
                               y_step * 5);
          ::SDL_RenderDrawLine(d_renderer, x_step + 1, y_step + 1,
                               x_step * 5 + 1, y_step * 5 + 1);
          break;
        case DIAGONAL_BOTTOMLEFT:
          // draw line
          ::SDL_SetRenderDrawColor(d_renderer, 0, 0, 255, 255);
          ::SDL_RenderDrawLine(d_renderer, x_step - 1, y_step * 5 - 1,
                               x_step * 5 - 1, y_step - 1);
          ::SDL_RenderDrawLine(d_renderer, x_step, y_step * 5, x_step * 5,
                               y_step);
          ::SDL_RenderDrawLine(d_renderer, x_step + 1, y_step * 5 + 1,
                               x_step * 5 + 1, y_step + 1);
          break;
      }

      if (d_gameWinner == CELL_X) {
        textColor(255, 255, 255);
        textCentered("X Won", d_display_width / 2, d_display_height / 2);
      }
      if (d_gameWinner == CELL_O) {
        textColor(255, 255, 255);
        textCentered("O Won", d_display_width / 2, d_display_height / 2);
      }
      if (d_gameWinner == CELL_EMPTY) {
        textColor(255, 255, 255);
        textCentered("Tie", d_display_width / 2, d_display_height / 2);
      }

      textColor(0, 255, 0, 0);
      textCentered("New Game [n] Quit[q]", d_display_width / 2,
                   d_display_height / 4);
    }
  }

  void textCentered(const char *text, int x, int y) {
    SDL_Surface *text_surf =
        ::TTF_RenderText_Solid(d_ttf_font, text, d_textColor);
    SDL_Texture *text_text =
        ::SDL_CreateTextureFromSurface(d_renderer, text_surf);

    SDL_Rect dest;
    dest.x = x - (text_surf->w / 2.0f);
    dest.y = y;
    dest.w = text_surf->w;
    dest.h = text_surf->h;
    ::SDL_RenderCopy(d_renderer, text_text, NULL, &dest);

    ::SDL_DestroyTexture(text_text);
    ::SDL_FreeSurface(text_surf);
  }

  void textColor(Uint8 r = 255, Uint8 g = 255, Uint8 b = 255, Uint8 a = 255) {
    d_textColor.r = r;
    d_textColor.g = g;
    d_textColor.b = b;
    d_textColor.a = a;
  }

  void background(Uint8 r = 0, Uint8 g = 0, Uint8 b = 0, Uint8 a = 255) {
    d_background.r = r;
    d_background.g = g;
    d_background.b = b;
    d_background.a = a;
  }

  void splashRender() {
    int x_center = d_display_width / 2;
    int y_row = d_display_height / 8;

    textColor(255, 255, 255);
    textCentered("Welcome to Tic Tac Toe", x_center, y_row * 1);

    textColor(32, 255, 0);
    if (d_firstMove != d_computerPlays) {
      textCentered("Human first [p,c]", x_center, y_row * 3);
    } else {
      textCentered("Computer first [p,c]", x_center, y_row * 3);
    }

    textColor(255, 0, 255);
    if (d_hardLevel) {
      textCentered("Hard Level [h,e]", x_center, y_row * 4);
    } else {
      textCentered("Easy Level [h,e]", x_center, y_row * 4);
    }

    textColor(255, 255, 255);
    textCentered("Press the SpaceBar to Play", x_center, y_row * 7);
  }

  void initGame() {
    d_currentPlayer = d_firstMove;

    for (int col = 0; col < NUM_COLS; col++) {
      for (int row = 0; row < NUM_ROWS; row++) {
        Cell &cell = d_board[col][row];

        cell.reset();
      }
    }
  }

  void gameResize() {
    int w = 0;
    int h = 0;
    SDL_GetRendererOutputSize(d_renderer, &w, &h);

    if (d_display_width != w || d_display_height != h) {
      d_display_width = w;
      d_display_height = h;
      std::cout << "gameResize" << " width " << d_display_width << " height "
                << d_display_height << std::endl;
      setBoardSize(w, h);
    }
  }

  int initialize() {
    ::SDL_Init(SDL_INIT_EVERYTHING);

    if (0 != ::TTF_Init()) {
      return -1;
    }

    d_window = ::SDL_CreateWindow("wasm-tic-tac-toe", SDL_WINDOWPOS_UNDEFINED,
                                  SDL_WINDOWPOS_UNDEFINED, d_display_width,
                                  d_display_height, 0);
    if (d_window == 0) {
      return -1;
    }

    d_renderer = ::SDL_CreateRenderer(d_window, -1, SDL_RENDERER_SOFTWARE);
    if (d_renderer == 0) {
      return -1;
    }

    d_ttf_font = ::TTF_OpenFontRW(
        ::SDL_RWFromMem(reinterpret_cast<void *>(const_cast<unsigned char *>(
                            &RobotoMono_Regular_ttf[0])),
                        sizeof(RobotoMono_Regular_ttf)),
        1, 36);

    d_RedX_Surface = ::SDL_LoadBMP_RW(
        ::SDL_RWFromMem(
            reinterpret_cast<void *>(const_cast<unsigned char *>(&RedX_bmp[0])),
            sizeof(RedX_bmp)),
        1);
    d_RedX_Texture = ::SDL_CreateTextureFromSurface(d_renderer, d_RedX_Surface);

    d_RedO_Surface = ::SDL_LoadBMP_RW(
        ::SDL_RWFromMem(
            reinterpret_cast<void *>(const_cast<unsigned char *>(&RedO_bmp[0])),
            sizeof(RedO_bmp)),
        1);
    d_RedO_Texture = ::SDL_CreateTextureFromSurface(d_renderer, d_RedO_Surface);
    return 0;
  }

  void finalize() {
    if (d_RedX_Surface) {
      ::SDL_FreeSurface(d_RedX_Surface);
      d_RedX_Surface = 0;
    }
    if (d_RedX_Texture) {
      ::SDL_DestroyTexture(d_RedX_Texture);
      d_RedX_Texture = 0;
    }
    if (d_RedO_Surface) {
      ::SDL_FreeSurface(d_RedO_Surface);
      d_RedO_Surface = 0;
    }
    if (d_RedO_Texture) {
      ::SDL_DestroyTexture(d_RedO_Texture);
      d_RedO_Texture = 0;
    }
    if (d_ttf_font) {
      ::TTF_CloseFont(d_ttf_font);
      d_ttf_font = 0;
    }
  }

  int gameLoop() {
    // Background
    ::SDL_SetRenderDrawColor(d_renderer, 0, 0, 0, 255);
    ::SDL_RenderClear(d_renderer);

    gameResize();

    // Draw border
    ::SDL_SetRenderDrawColor(d_renderer, 64, 64, 64, 255);
    ::SDL_RenderDrawRect(d_renderer, NULL);

    SDL_Event event;
    while (::SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_QUIT:
          return -1;
        case SDL_KEYDOWN:
          if (event.key.keysym.sym == SDLK_ESCAPE) {
            return -1;
          }
          keyPressed(event);
          break;

        case SDL_MOUSEBUTTONDOWN:
          mousePressed(event);
          break;
      }
    }

    if (!d_gameInProgress) {
      splashRender();
    } else {
      makeComputerMove();
      boardRender();
      boardWinnerRender();
    }

    ::SDL_RenderPresent(d_renderer);
    return 0;
  }
};

#ifdef __EMSCRIPTEN__
void main_loop(void *user_data) {
  TicTacToe *ticTacToe = (TicTacToe *)user_data;
  if (0 != ticTacToe->gameLoop()) {
    // emscripten_cancel_main_loop();
  }
}
#endif
}  // namespace

int main(int argc, char **argv) {
  TicTacToe ticTacToe;

  ticTacToe.initialize();

#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop_arg(main_loop, &ticTacToe, 0, 1);
#else
  while (0 == ticTacToe.gameLoop()) {
  }

  ticTacToe.finalize();
#endif
}
