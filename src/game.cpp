#include "game.h"

namespace ttt {

Game::Game() = default;

Game::Game(unsigned seed) : d_ai(seed) {}

void Game::setComputerFirst(bool computerFirst) {
  d_first = computerFirst ? d_computer : other(d_computer);
}

void Game::setHard(bool hard) { d_hard = hard; }

void Game::start() {
  d_playing = true;
  d_board.clear();
  d_current = d_first;
  d_result = Result{};
}

void Game::quit() {
  d_playing = false;
  d_board.clear();
  d_result = Result{};
}

void Game::play(int idx, Mark m) {
  d_board.set(idx, m);
  d_current = other(m);
  d_result = d_board.result();
}

bool Game::humanPlays(int idx) {
  if (!d_playing || d_result.finished || d_current == d_computer || idx < 0 ||
      idx >= kCells || d_board.at(idx) != Mark::Empty) {
    return false;
  }
  play(idx, d_current);
  return true;
}

const char *Game::computerTurn() {
  if (!d_playing || d_result.finished || d_current != d_computer) {
    return nullptr;
  }
  const Move move = d_hard ? d_ai.advancedMove(d_board, d_computer)
                           : d_ai.randomMove(d_board);
  play(move.idx, d_computer);
  return move.reason;
}

}  // namespace ttt
