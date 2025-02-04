#ifndef __INCLUDE_GUARD_ENGINE_AGENT_HPP
#define __INCLUDE_GUARD_ENGINE_AGENT_HPP

#include <cmath>
#include <cstdint>
#include <iostream>
#include <random>

#include "engine_GameState.hpp"
#include "util_TimeStamp.hpp"

namespace engine {
class Agent {
 public:
  virtual ~Agent() = default;

  void SetStreams(std::istream& input, std::ostream& output) {
    input_ = &input;
    output_ = &output;
  }

  virtual void Init() { GameState::ReadBoard(*input_, arid_); }

  Move Turn() {
    util::TimeStamp start;
    GameState state = GameState::FromStream(*input_, start, arid_);

    Move move = ChooseMove(state, start);

    // std::cerr << "D: " << start.Since() << std::endl;

    *output_ << move << std::endl;
    return move;
  }

  virtual Move ChooseMove(GameState const& state,
                          util::TimeStamp const& start) = 0;

  int Rand() { return dist_(rand_engine_); }
  uint64_t GetArid() const { return arid_; }

 private:
  std::istream* input_{};
  std::ostream* output_{};
  uint64_t arid_{};
  std::default_random_engine rand_engine_{std::random_device{}()};
  std::uniform_int_distribution<int> dist_{0, RAND_MAX};
};

}  // namespace engine

#endif /* __INCLUDE_GUARD_ENGINE_AGENT_HPP */