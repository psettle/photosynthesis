#ifndef __INCLUDE_GUARD_AGENT_NEURAL_HEURISTIC_HPP
#define __INCLUDE_GUARD_AGENT_NEURAL_HEURISTIC_HPP

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "agent_DataPoint.hpp"
#include "engine_GameState.hpp"
#include "engine_Referee.hpp"
#include "neural_Linear.hpp"
#include "neural_MeanSquareError.hpp"
#include "neural_Network.hpp"
#include "neural_ReLU.hpp"

namespace agent {

class NeuralHeuristic {
 public:
  static size_t constexpr kInputDimensions =
      engine::N_TREES + engine::N_TREES + engine::N_TREES + 7;

  NeuralHeuristic() {
    std::vector<std::unique_ptr<neural::ILayer>> layers;
    layers.emplace_back(std::make_unique<neural::Linear>(kInputDimensions, 32));
    layers.emplace_back(std::make_unique<neural::ReLU>());
    layers.emplace_back(std::make_unique<neural::Linear>(32, 24));
    layers.emplace_back(std::make_unique<neural::ReLU>());
    layers.emplace_back(std::make_unique<neural::Linear>(24, 16));
    layers.emplace_back(std::make_unique<neural::ReLU>());
    layers.emplace_back(std::make_unique<neural::Linear>(16, 1));
    network_ = std::make_unique<neural::Network>(
        std::move(layers), std::make_unique<neural::MeanSquareError>());
  }

  NeuralHeuristic(std::unique_ptr<neural::Network> network)
      : network_(std::move(network)) {}

  NeuralHeuristic(std::string const& fname)
      : network_(std::make_unique<neural::Network>(
            neural::Network::LoadFromFile(fname))) {}

  float Evaluate(engine::GameState const& g, uint64_t arid) const {
    neural::FloatTensor features(1, NeuralHeuristic::kInputDimensions);
    NeuralHeuristic::ToFeatures(features, 0, g, arid);
    return const_cast<neural::Network const*>(network_.get())
        ->Forward(std::move(features))
        .value();
  }

  std::unique_ptr<NeuralHeuristic> Clone() const {
    return std::make_unique<NeuralHeuristic>(network_->Clone());
  }

  neural::Network& network() { return *network_; }
  neural::Network const& network() const { return *network_; }

  static neural::FloatTensor ToFeatures(
      std::vector<engine::Episode::Frame const*> const& data) {
    neural::FloatTensor result(data.size(), kInputDimensions);

    for (size_t r = 0; r < data.size(); ++r) {
      ToFeatures(result, r, data[r]->state, data[r]->arid);
    }

    return result;
  }

  static void ToFeatures(neural::FloatTensor& tensor, u_int r,
                         engine::GameState const& g, uint64_t arid) {
    size_t c = 0u;

    for (u_int t = 0; t < engine::N_TREES; ++t) {
      float tree_size = 0.0f;
      for (u_int s = 0; s < 4; ++s) {
        if (g.GetPlayerTrees(0, s) & engine::GetTree(t)) {
          tree_size = (s + 1) * 0.25f;
        } else if (g.GetPlayerTrees(1, s) & engine::GetTree(t)) {
          tree_size = (s + 1) * -0.25f;
        }
      }

      tensor.Get(r, c++) = tree_size;
    }

    for (u_int t = 0; t < engine::N_TREES; ++t) {
      if (g.GetDormant() & engine::GetTree(t)) {
        tensor.Get(r, c++) = 1.0f;
      } else {
        tensor.Get(r, c++) = -1.0f;
      }
    }

    for (u_int t = 0; t < engine::N_TREES; ++t) {
      if (arid & engine::GetTree(t)) {
        tensor.Get(r, c++) = 1.0f;
      } else {
        tensor.Get(r, c++) = -1.0f;
      }
    }

    tensor.Get(r, c++) = g.GetDay() / 24.0f - 0.5f;
    tensor.Get(r, c++) = g.GetNutrients() / 20.0f - 0.5f;
    tensor.Get(r, c++) = g.GetScore(0) / 100.0f - 0.5f;
    tensor.Get(r, c++) = g.GetSun(0) / 40.0f - 0.5f;
    tensor.Get(r, c++) = g.GetScore(1) / 100.0f - 0.5f;
    tensor.Get(r, c++) = g.GetSun(1) / 40.0f - 0.5f;
    tensor.Get(r, c++) = (g.GetDay() % 6) / 5.0 - 5.0;
  }

  static neural::FloatTensor ToExpected(
      std::vector<engine::Episode::Frame const*> const& data) {
    neural::FloatTensor result(data.size(), 1);

    for (u_int r = 0; r < data.size(); ++r) {
      switch (data[r]->winner) {
        case 0:
          result.Get(r, 0) = 1.0f;
          break;
        case 1:
          result.Get(r, 0) = -1.0f;
          break;
        case 2:
          result.Get(r, 0) = 0.0f;
          break;
      }
    }

    return result;
  }

 private:
  std::unique_ptr<neural::Network> network_;
};

}  // namespace agent

#endif /* __INCLUDE_GUARD_AGENT_NEURAL_HEURISTIC_HPP */