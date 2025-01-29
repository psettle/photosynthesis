#pragma GCC optimize "O3,omit-frame-pointer,inline"
#include <iostream>

#include "agent_Mcts.hpp"

int main() {
  agent::Mcts agent;
  agent.SetStreams(std::cin, std::cout);
  agent.Init();

  while (true) {
    agent.Turn();
  }

  return 0;
}
