#include <algorithm>
#include <random>

#include "agent_DataPoint.hpp"
#include "agent_NeuralHeuristic.hpp"

size_t constexpr BATCH_SIZE = 128;
float constexpr LEARN_RATE = 0.05f;

int main() {
  auto data = agent::LoadDataPoints("photosynthesis_expert_players.bin");

  std::cout << "Data Loaded: " << data.size() << " samples" << std::endl;
  auto rand_fn = std::default_random_engine{std::random_device{}()};

  size_t training_count = static_cast<size_t>(std::round(data.size() * 0.90));
  std::cout << "Training count: " << training_count << std::endl;
  std::cout << "Eval count: " << (data.size() - training_count) << std::endl;

  auto training = std::vector<agent::DataPoint>(data.begin(),
                                                data.begin() + training_count);
  auto eval =
      std::vector<agent::DataPoint>(data.begin() + training_count, data.end());

  std::cout << "Data prepared." << std::endl;

  agent::NeuralHeuristic heuristic;
  auto& network = heuristic.network();

  std::cout << "Network prepared." << std::endl;

  for (size_t e = 0; true; ++e) {
    std::shuffle(training.begin(), training.end(), rand_fn);

    float loss_sum = 0.0f;
    size_t n_batches = 0u;

    util::TimeStamp start;
    for (size_t i = BATCH_SIZE; i < training.size(); i += BATCH_SIZE) {
      agent::DataPoint const* batch = training.data() + i - BATCH_SIZE;

      auto input = agent::NeuralHeuristic::ToFeatures(batch, BATCH_SIZE);
      auto expected = agent::NeuralHeuristic::ToExpected(batch, BATCH_SIZE);

      auto loss = network.Batch(input, expected, LEARN_RATE);
      n_batches++;
      loss_sum += loss;
    }

    auto input = agent::NeuralHeuristic::ToFeatures(eval.data(), eval.size());
    auto expected =
        agent::NeuralHeuristic::ToExpected(eval.data(), eval.size());
    auto loss = network.Loss(input, expected);

    std::cout << e << ": train=" << std::sqrt(loss_sum / n_batches)
              << " eval=" << std::sqrt(loss) << " t=" << start.Since()
              << std::endl;

    if (e % 10 == 0) {
      network.SaveToFile("network_" + std::to_string(e) + ".bin");
    }
  }
}