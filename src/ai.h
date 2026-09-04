// Computer opponent. Seeded from std::random_device by default so no two
// games play the same "random" sequence; pass a seed for reproducible tests.
#ifndef WASM_TIC_TAC_TOE_AI_H
#define WASM_TIC_TAC_TOE_AI_H

#include <random>
#include <vector>

#include "board.h"

namespace ttt {

struct Move {
  int idx;             // cell to play, always empty on the given board
  const char *reason;  // short description for the console log
};

class Ai {
 public:
  Ai();
  explicit Ai(unsigned seed);

  // Any empty cell. The board must not be full.
  Move randomMove(const Board &board);

  // Win if possible, else block, else corner / center / side.
  // The board must not be full.
  Move advancedMove(const Board &board, Mark player);

 private:
  int pick(const std::vector<int> &cells);

  std::mt19937 d_rng;
};

}  // namespace ttt

#endif  // WASM_TIC_TAC_TOE_AI_H
