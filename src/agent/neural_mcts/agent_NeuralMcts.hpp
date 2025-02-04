#ifndef __INCLUDE_GUARD_AGENT_NEURALMCTS_HPP
#define __INCLUDE_GUARD_AGENT_NEURALMCTS_HPP

#include "agent_Mcts.hpp"
#include "agent_NeuralHeuristic.hpp"

namespace agent {

class NeuralMcts : public Mcts {
 public:
  NeuralMcts(NeuralHeuristic const* network, float epsilon)
      : Mcts(), network_(network), epsilon_(epsilon) {}

  float Heuristic(GameState const& gs) override {
    return network_->Evaluate(gs, GetArid());
  }

  bool CheckLimit(TimeStamp const& start, size_t n_rollouts,
                  bool first_turn) override {
    return n_rollouts > 1600;
  }

  Move ChooseMove(GameState const& state, TimeStamp const& start) override {
    if (RandF() < epsilon_) {
      ResetHistory();
      auto params = GameState::MoveFilterParams::Default();
      params.block_self_neighbor = false;
      auto moves = state.GetMoves(0, GetArid(), params);
      return moves[Rand() % moves.size()];
    }

    return Mcts::ChooseMove(state, start);
  }

 private:
  float RandF() { return Rand() / static_cast<float>(RAND_MAX); }

  NeuralHeuristic const* network_;
  float epsilon_;
};
}  // namespace agent

#endif