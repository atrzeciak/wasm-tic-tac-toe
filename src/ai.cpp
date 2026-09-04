#include "ai.h"

#include <optional>

namespace ttt {
namespace {

// The first empty cell that completes a line for `m`, if any.
std::optional<int> completingCell(const Board &board, Mark m) {
  Board scratch = board;
  for (const int idx : board.empties()) {
    scratch.set(idx, m);
    if (scratch.winningLine(m)) {
      return idx;
    }
    scratch.set(idx, Mark::Empty);
  }
  return std::nullopt;
}

template <typename Cells>
std::vector<int> emptyOf(const Board &board, const Cells &cells) {
  std::vector<int> out;
  for (const int idx : cells) {
    if (board.at(idx) == Mark::Empty) {
      out.push_back(idx);
    }
  }
  return out;
}

}  // namespace

Ai::Ai() : Ai(std::random_device{}()) {}

Ai::Ai(unsigned seed) : d_rng(seed) {}

int Ai::pick(const std::vector<int> &cells) {
  std::uniform_int_distribution<size_t> dist(0, cells.size() - 1);
  return cells[dist(d_rng)];
}

Move Ai::randomMove(const Board &board) {
  return Move{pick(board.empties()), "Random move"};
}

Move Ai::advancedMove(const Board &board, Mark player) {
  if (const std::optional<int> idx = completingCell(board, player)) {
    return Move{*idx, "Make a winning move"};
  }
  if (const std::optional<int> idx = completingCell(board, other(player))) {
    return Move{*idx, "Block a player"};
  }

  const std::vector<int> corners = emptyOf(board, kCorners);
  if (corners.size() == kCorners.size()) {
    return Move{pick(corners), "Take a corner"};
  }
  if (board.at(kCenter) == Mark::Empty) {
    return Move{kCenter, "Take the center"};
  }
  const std::vector<int> sides = emptyOf(board, kSides);
  if (!sides.empty()) {
    return Move{pick(sides), "Choose a random side"};
  }
  if (!corners.empty()) {
    return Move{pick(corners), "Take a corner"};
  }
  return randomMove(board);
}

}  // namespace ttt
