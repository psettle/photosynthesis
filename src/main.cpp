#include <deque>
#include <fstream>
#include <iostream>

#include "agent_Mcts.hpp"
#include "agent_NeuralMcts.hpp"
#include "test_Runner.hpp"

template <class AgentClass>
class SimpleAgentFactory : public engine::IAgentFactory {
 public:
  std::unique_ptr<engine::Agent> MakeAgent() const override {
    return std::make_unique<AgentClass>();
  }
};

class MctsFactory : public SimpleAgentFactory<agent::Mcts> {};

static float constexpr EPSILON = 0.05f;

class NeuralAgentFactory : public engine::IAgentFactory {
 public:
  NeuralAgentFactory(agent::NeuralHeuristic const* network)
      : network_(network) {}

  std::unique_ptr<engine::Agent> MakeAgent() const override {
    return std::make_unique<agent::NeuralMcts>(network_, e_);
  }

  void SetEpsilon(float e) { e_ = e; }

 private:
  agent::NeuralHeuristic const* network_;
  float e_{EPSILON};
};

static std::deque<engine::Episode::Frame> training_samples;
static size_t constexpr BATCH_SIZE = 2048;
static float constexpr LEARN_RATE = 0.01f;

static size_t constexpr N_SAMPLE_EPISODE = 1024;
static size_t constexpr N_TEST_EPISODE = 512;
static size_t constexpr N_EVAL_EPISODE = 64;
static size_t constexpr N_MAX_TRAINING_FRAMES = 1000000;
static size_t constexpr N_TRAIN_STEPS = 2048;

int main() {
  size_t count = 0;

  auto best = std::make_unique<agent::NeuralHeuristic>();
  auto learning = std::make_unique<agent::NeuralHeuristic>();
  auto best_factory = std::make_unique<NeuralAgentFactory>(best.get());
  auto learn_factory = std::make_unique<NeuralAgentFactory>(learning.get());

  MctsFactory benchmark_factory{};

  u_int epoch = 0;
  while (true) {
    util::TimeStamp start;

    best_factory->SetEpsilon(EPSILON);
    auto episodes =
        test::Test(*best_factory, *best_factory, N_SAMPLE_EPISODE, 16);

    for (auto const& e : episodes) {
      for (auto const& f : e.episode) {
        training_samples.emplace_back(f);
      }
    }

    while (training_samples.size() > N_MAX_TRAINING_FRAMES) {
      training_samples.pop_front();
    }

    std::default_random_engine e(std::random_device{}());
    std::uniform_int_distribution<size_t> uni_train(0, training_samples.size());
    auto rand_fn = [&]() { return uni_train(e); };

    float loss_sum = 0.0f;
    for (u_int b = 0; b < N_TRAIN_STEPS; ++b) {
      std::vector<engine::Episode::Frame const*> batch(BATCH_SIZE);
      for (u_int i = 0; i < BATCH_SIZE; ++i) {
        batch[i] = &training_samples[rand_fn()];
      }

      auto features = agent::NeuralHeuristic::ToFeatures(batch);
      auto truth = agent::NeuralHeuristic::ToExpected(batch);
      float loss = learning->network().Batch(features, truth, LEARN_RATE);
      loss_sum += loss;
    }

    best_factory->SetEpsilon(0.0f);
    learn_factory->SetEpsilon(0.0f);

    auto test_episodes =
        test::Test(*learn_factory, *best_factory, N_TEST_EPISODE, 16);
    u_int learn_win = 0;
    u_int best_win = 0;
    for (auto const& e : test_episodes) {
      if (e.episode[0].winner == 0) {
        learn_win++;
      } else if (e.episode[0].winner == 1) {
        best_win++;
      }
    }

    test_episodes =
        test::Test(*learn_factory, benchmark_factory, N_EVAL_EPISODE, 16);
    int64_t mcts_delta = 0;
    for (auto const& e : test_episodes) {
      auto const& s = e.episode.back().state;
      mcts_delta += static_cast<int64_t>(s.GetScore(0)) +
                    static_cast<int64_t>(s.GetSun(0)) / 3;
      mcts_delta -= static_cast<int64_t>(s.GetScore(1)) +
                    static_cast<int64_t>(s.GetSun(1)) / 3;
    }

    std::cout << epoch << ": train=" << std::sqrt(loss_sum / N_TRAIN_STEPS)
              << " vs_best_wins=" << learn_win << "/" << N_TEST_EPISODE
              << " vs_mcts_wins="
              << (mcts_delta / static_cast<float>(N_EVAL_EPISODE)) << "/"
              << N_EVAL_EPISODE << " time=" << start.Since() << std::endl;
    if (learn_win > best_win + 60) {
      std::cout << "Swapping Model!" << std::endl;
      best = learning->Clone();
      best_factory = std::make_unique<NeuralAgentFactory>(best.get());
      best->network().SaveToFile("model_swap_" + std::to_string(epoch) +
                                 ".bin");
    }

    epoch++;
  }

  std::cout << count << std::endl;
}