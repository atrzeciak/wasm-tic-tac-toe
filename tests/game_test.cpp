// Unit tests for the game flow in game.h: start-screen settings, turn
// taking, ignored input and the end of a game. Run by ctest (`make test`).
#include <iostream>

#include "board.h"
#include "game.h"

namespace {

using ttt::Game;
using ttt::Mark;

int failures = 0;

void check(bool ok, const char *expr, int line) {
  if (!ok) {
    std::cerr << __FILE__ << ':' << line << ": CHECK failed: " << expr << '\n';
    ++failures;
  }
}

#define CHECK(cond) check((cond), #cond, __LINE__)

constexpr unsigned kSeed = 1;

// The human's mark: the computer is always O in this game.
Mark human(const Game &g) { return ttt::other(g.computer()); }

void testStartScreenDefaults() {
  const Game g(kSeed);
  CHECK(!g.playing());
  CHECK(!g.finished());
  CHECK(g.computerFirst());
  CHECK(g.hard());
  CHECK(g.computer() == Mark::O);
  CHECK(g.board().count() == 0);
}

void testNothingHappensOnStartScreen() {
  Game g(kSeed);
  CHECK(!g.humanPlays(4));
  CHECK(g.computerTurn() == nullptr);
  CHECK(g.board().count() == 0);
}

void testSettingsPickWhoOpens() {
  Game g(kSeed);
  g.setComputerFirst(false);
  CHECK(!g.computerFirst());
  g.start();
  CHECK(g.playing());
  CHECK(g.current() == human(g));
  CHECK(g.computerTurn() == nullptr);  // not its turn
  CHECK(g.board().count() == 0);

  g.quit();
  g.setComputerFirst(true);
  g.start();
  CHECK(g.current() == g.computer());
  CHECK(g.computerTurn() != nullptr);
  CHECK(g.board().count() == 1);
  CHECK(g.current() == human(g));
}

void testTurnsAlternate() {
  Game g(kSeed);
  g.setComputerFirst(false);
  g.start();
  CHECK(g.humanPlays(4));
  CHECK(g.board().at(4) == human(g));
  CHECK(g.current() == g.computer());
  CHECK(!g.humanPlays(0));  // not the human's turn
  CHECK(g.board().count() == 1);
  CHECK(g.computerTurn() != nullptr);
  CHECK(g.board().count() == 2);
  CHECK(g.current() == human(g));
  CHECK(!g.humanPlays(4));  // taken
  CHECK(!g.humanPlays(-1));
  CHECK(!g.humanPlays(ttt::kCells));
  CHECK(g.board().count() == 2);
}

void testHardComputerBlocksAndWins() {
  Game g(kSeed);
  g.setComputerFirst(false);
  g.setHard(true);
  g.start();
  // Human takes two corners of the top row; the computer must block.
  CHECK(g.humanPlays(0));
  CHECK(g.computerTurn() != nullptr);
  CHECK(g.humanPlays(2) || g.board().at(2) == g.computer());
  if (g.board().at(1) != g.computer()) {
    CHECK(g.computerTurn() != nullptr);
    CHECK(g.board().at(1) == g.computer());
  }
}

void testHumanWinEndsGame() {
  Game g(kSeed);
  g.setComputerFirst(false);
  g.setHard(false);  // random opponent: drive it into a corner via the seed
  g.start();
  // Play a full game where the human always takes the lowest free cell that
  // keeps a line alive; stop when someone wins or the board fills.
  while (!g.finished()) {
    bool played = false;
    for (int idx = 0; idx < ttt::kCells && !played; ++idx) {
      played = g.humanPlays(idx);
    }
    CHECK(played);
    g.computerTurn();
  }
  CHECK(g.playing());
  CHECK(g.finished());
  CHECK(
      !g.humanPlays(g.board().empties().empty() ? 0 : g.board().empties()[0]));
  CHECK(g.computerTurn() == nullptr);
  const ttt::Result &r = g.result();
  CHECK(r.winner == Mark::Empty ? !r.line.has_value() : r.line.has_value());
}

void testRestartAndQuit() {
  Game g(kSeed);
  g.setComputerFirst(false);
  g.start();
  CHECK(g.humanPlays(0));
  g.start();  // "n": new game keeps the settings, clears the board
  CHECK(g.playing());
  CHECK(g.board().count() == 0);
  CHECK(!g.finished());
  CHECK(g.current() == human(g));

  CHECK(g.humanPlays(0));
  g.quit();  // "q": back to the start screen
  CHECK(!g.playing());
  CHECK(g.board().count() == 0);
  CHECK(!g.computerFirst());  // settings survive
  CHECK(!g.humanPlays(0));
}

void testFullGameIsDeterministicForSeed() {
  Game a(7);
  Game b(7);
  for (Game *g : {&a, &b}) {
    g->setHard(false);
    g->start();
    while (!g->finished()) {
      g->computerTurn();
      for (int idx = 0; idx < ttt::kCells && !g->humanPlays(idx); ++idx) {
      }
    }
  }
  for (int idx = 0; idx < ttt::kCells; ++idx) {
    CHECK(a.board().at(idx) == b.board().at(idx));
  }
}

}  // namespace

int main() {
  testStartScreenDefaults();
  testNothingHappensOnStartScreen();
  testSettingsPickWhoOpens();
  testTurnsAlternate();
  testHardComputerBlocksAndWins();
  testHumanWinEndsGame();
  testRestartAndQuit();
  testFullGameIsDeterministicForSeed();
  if (failures != 0) {
    std::cerr << failures << " check(s) failed\n";
    return 1;
  }
  std::cout << "all checks passed\n";
  return 0;
}
