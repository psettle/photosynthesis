#include <fstream>
#include <iostream>

#include "agent_DataPoint.hpp"
#include "agent_Mcts.hpp"
#include "test_Runner.hpp"

int main() {
  std::ofstream data_stream;
  data_stream.open("photosynthesis_expert_players.bin");

  size_t count = 0;
  auto result = test::Test<agent::Mcts, agent::Mcts>(
      8192, 16, 100,
      [&](std::vector<engine::GameState> const& states, u_int p0_score,
          u_int p1_score, u_int winner, uint64_t arid) {
        for (auto const& s : states) {
          agent::DataPoint d{s, arid, static_cast<uint8_t>(p0_score),
                             static_cast<uint8_t>(p1_score),
                             static_cast<uint8_t>(winner)};

          d.Serialize(data_stream);
        }

        count += states.size();
      });

  data_stream.close();

  std::cout << result.n_games << std::endl;
  std::cout << result.left_wins << std::endl;
  std::cout << result.right_wins << std::endl;
  std::cout << count << std::endl;
}