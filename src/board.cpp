#include "board.h"

#include <algorithm>
#include <optional>
#include <vector>

namespace ttt {

bool Board::full() const {
  return std::none_of(d_cells.begin(), d_cells.end(),
                      [](Mark m) { return m == Mark::Empty; });
}

int Board::count() const {
  return static_cast<int>(std::count_if(
      d_cells.begin(), d_cells.end(), [](Mark m) { return m != Mark::Empty; }));
}

std::vector<int> Board::empties() const {
  std::vector<int> out;
  for (int idx = 0; idx < kCells; ++idx) {
    if (at(idx) == Mark::Empty) {
      out.push_back(idx);
    }
  }
  return out;
}

std::optional<Line> Board::winningLine(Mark m) const {
  for (const Line &line : kLines) {
    if (std::all_of(line.begin(), line.end(),
                    [&](int idx) { return at(idx) == m; })) {
      return line;
    }
  }
  return std::nullopt;
}

Result Board::result() const {
  for (const Mark m : {Mark::X, Mark::O}) {
    if (const std::optional<Line> line = winningLine(m)) {
      return Result{true, m, line};
    }
  }
  if (full()) {
    return Result{true, Mark::Empty, std::nullopt};
  }
  return Result{};
}

}  // namespace ttt
