// Tic-Tac-Toe rules and board state. Pure C++, no SDL: this is what the unit
// tests in tests/ exercise and what the AI in ai.h plays on.
#ifndef WASM_TIC_TAC_TOE_BOARD_H
#define WASM_TIC_TAC_TOE_BOARD_H

#include <array>
#include <optional>
#include <vector>

namespace ttt {

constexpr int kSize = 3;
constexpr int kCells = kSize * kSize;

enum class Mark : char { Empty = ' ', X = 'X', O = 'O' };

constexpr Mark other(Mark m) { return m == Mark::X ? Mark::O : Mark::X; }

// Cells are indexed 0..8 in row-major order:
//   0 1 2
//   3 4 5
//   6 7 8
constexpr int cellIndex(int col, int row) { return col + kSize * row; }
constexpr int cellCol(int idx) { return idx % kSize; }
constexpr int cellRow(int idx) { return idx / kSize; }

// Three cell indices that form a winning line.
using Line = std::array<int, kSize>;

// Every winning line: rows, columns, then the two diagonals.
constexpr std::array<Line, 8> kLines = {{
    {0, 1, 2},
    {3, 4, 5},
    {6, 7, 8},
    {0, 3, 6},
    {1, 4, 7},
    {2, 5, 8},
    {0, 4, 8},
    {2, 4, 6},
}};

constexpr std::array<int, 4> kCorners = {0, 2, 6, 8};
constexpr std::array<int, 4> kSides = {1, 3, 5, 7};
constexpr int kCenter = 4;

struct Result {
  bool finished = false;
  Mark winner = Mark::Empty;  // Mark::Empty with finished == true is a draw
  std::optional<Line> line;   // the winning line, if any
};

class Board {
 public:
  [[nodiscard]] Mark at(int idx) const {
    return d_cells.at(static_cast<size_t>(idx));
  }
  void set(int idx, Mark m) { d_cells.at(static_cast<size_t>(idx)) = m; }
  void clear() { d_cells = emptyCells(); }

  [[nodiscard]] bool full() const;
  [[nodiscard]] int count() const;
  [[nodiscard]] std::vector<int> empties() const;

  [[nodiscard]] std::optional<Line> winningLine(Mark m) const;
  [[nodiscard]] Result result() const;

 private:
  static constexpr std::array<Mark, kCells> emptyCells() {
    std::array<Mark, kCells> cells{};
    for (Mark &m : cells) {
      m = Mark::Empty;
    }
    return cells;
  }

  std::array<Mark, kCells> d_cells = emptyCells();
};

}  // namespace ttt

#endif  // WASM_TIC_TAC_TOE_BOARD_H
