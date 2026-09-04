// The game's flow: start screen settings, turn taking and the end of a game.
// No SDL here; the front end in main.cpp translates input into these calls
// and draws from the accessors, and tests/game_test.cpp drives it directly.
#ifndef WASM_TIC_TAC_TOE_GAME_H
#define WASM_TIC_TAC_TOE_GAME_H

#include "ai.h"
#include "board.h"

namespace ttt {

class Game {
 public:
  Game();
  explicit Game(unsigned seed);  // reproducible computer moves

  // ---- start screen ----
  void setComputerFirst(bool computerFirst);
  void setHard(bool hard);
  // Clears the board and starts playing; the side chosen with
  // setComputerFirst opens. Also restarts a game in progress.
  void start();
  // Abandons the game and returns to the start screen.
  void quit();

  // ---- playing ----
  // The human marks cell `idx`. Ignored, returning false, unless a game is
  // in progress, unfinished, it is the human's turn and the cell is empty.
  bool humanPlays(int idx);
  // The computer moves if a game is in progress, unfinished and it is its
  // turn; returns why it chose that cell, or nullptr when it did not move.
  const char *computerTurn();

  // ---- observers ----
  [[nodiscard]] bool playing() const { return d_playing; }
  [[nodiscard]] bool finished() const { return d_result.finished; }
  [[nodiscard]] bool computerFirst() const { return d_first == d_computer; }
  [[nodiscard]] bool hard() const { return d_hard; }
  [[nodiscard]] Mark computer() const { return d_computer; }
  [[nodiscard]] Mark current() const { return d_current; }
  [[nodiscard]] const Board &board() const { return d_board; }
  [[nodiscard]] const Result &result() const { return d_result; }

 private:
  void play(int idx, Mark m);

  Board d_board;
  Ai d_ai;
  Mark d_computer = Mark::O;
  Mark d_first = Mark::O;
  bool d_hard = true;
  Mark d_current = Mark::O;
  bool d_playing = false;
  Result d_result;
};

}  // namespace ttt

#endif  // WASM_TIC_TAC_TOE_GAME_H
