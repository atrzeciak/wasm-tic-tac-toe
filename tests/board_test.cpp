// Unit tests for the SDL-free game logic. Plain checks, no framework: run by
// ctest from the native build tree (`make test`).
#include <algorithm>
#include <iostream>
#include <string_view>

#include "ai.h"
#include "board.h"

namespace {

using ttt::Ai;
using ttt::Board;
using ttt::Line;
using ttt::Mark;

int failures = 0;

void check(bool ok, const char *expr, int line) {
  if (!ok) {
    std::cerr << __FILE__ << ':' << line << ": CHECK failed: " << expr << '\n';
    ++failures;
  }
}

#define CHECK(cond) check((cond), #cond, __LINE__)

// Builds a board from nine characters in row-major order, e.g. "XO X  O  ".
Board board(std::string_view cells) {
  Board b;
  for (int idx = 0; idx < ttt::kCells; ++idx) {
    b.set(idx, static_cast<Mark>(cells[static_cast<size_t>(idx)]));
  }
  return b;
}

template <typename Cells>
bool contains(const Cells &cells, int idx) {
  return std::find(cells.begin(), cells.end(), idx) != cells.end();
}

void testEmptyBoard() {
  const Board b;
  CHECK(b.count() == 0);
  CHECK(!b.full());
  CHECK(b.empties().size() == ttt::kCells);
  CHECK(!b.winningLine(Mark::X));
  CHECK(!b.winningLine(Mark::O));
  CHECK(!b.result().finished);
}

void testLineWins(const Line &line) {
  Board b;
  for (const int idx : line) {
    b.set(idx, Mark::X);
  }
  CHECK(b.winningLine(Mark::X) == line);
  CHECK(!b.winningLine(Mark::O));
  const ttt::Result r = b.result();
  CHECK(r.finished);
  CHECK(r.winner == Mark::X);
  CHECK(r.line == line);
}

void testEveryLineWins() {
  for (const Line &line : ttt::kLines) {
    testLineWins(line);
  }
}

void testDraw() {
  const Board b = board(
      "XOX"
      "XOO"
      "OXX");
  CHECK(b.full());
  CHECK(b.count() == ttt::kCells);
  const ttt::Result r = b.result();
  CHECK(r.finished);
  CHECK(r.winner == Mark::Empty);
  CHECK(!r.line);
}

void testInProgress() {
  const Board b = board(
      "XO "
      " X "
      "  O");
  CHECK(b.count() == 4);
  CHECK(b.empties().size() == 5);
  CHECK(!b.result().finished);
}

void testAiWinsWhenPossible() {
  Ai ai(1);
  const Board b = board(
      "XX "
      "OO "
      "   ");
  CHECK(ai.advancedMove(b, Mark::X).idx == 2);
  CHECK(ai.advancedMove(b, Mark::O).idx == 5);
}

void testAiBlocks() {
  Ai ai(1);
  const Board b = board(
      "XX "
      "O  "
      "   ");
  CHECK(ai.advancedMove(b, Mark::O).idx == 2);
}

void testAiOpening() {
  Ai ai(1);
  const Board empty;
  CHECK(contains(ttt::kCorners, ai.advancedMove(empty, Mark::O).idx));

  // A corner is taken, so the centre is next.
  CHECK(ai.advancedMove(board("X        "), Mark::O).idx == ttt::kCenter);

  // Centre and a corner taken: a side.
  CHECK(
      contains(ttt::kSides, ai.advancedMove(board("O   X    "), Mark::O).idx));
}

void testAiTakesLastCell() {
  Ai ai(1);
  const Board b = board(
      "XOX"
      "XOO"
      "OX ");
  CHECK(ai.advancedMove(b, Mark::X).idx == 8);
  CHECK(ai.randomMove(b).idx == 8);
}

void testRandomMoveIsLegal() {
  Ai ai(7);
  const Board b = board(
      "X O"
      " X "
      "O  ");
  for (int i = 0; i < 100; ++i) {
    const int idx = ai.randomMove(b).idx;
    CHECK(contains(b.empties(), idx));
  }
}

void testSeedIsReproducible() {
  Ai a(42);
  Ai b(42);
  const Board empty;
  for (int i = 0; i < 20; ++i) {
    CHECK(a.randomMove(empty).idx == b.randomMove(empty).idx);
  }
}

}  // namespace

int main() {
  testEmptyBoard();
  testEveryLineWins();
  testDraw();
  testInProgress();
  testAiWinsWhenPossible();
  testAiBlocks();
  testAiOpening();
  testAiTakesLastCell();
  testRandomMoveIsLegal();
  testSeedIsReproducible();
  if (failures != 0) {
    std::cerr << failures << " check(s) failed\n";
    return 1;
  }
  std::cout << "all checks passed\n";
  return 0;
}
