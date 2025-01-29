#include <iostream>

#include "agent_Mcts.hpp"
#include "test_Runner.hpp"

int main() {
  auto result = test::Test<agent::Mcts, agent::Mcts>(2048, 16);
  std::cout << result.n_games << std::endl;
  std::cout << result.left_wins << std::endl;
  std::cout << result.right_wins << std::endl;
}