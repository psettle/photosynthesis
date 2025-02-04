#ifndef __INCLUDE_GUARD_ENGINE_REFEREE_HPP
#define __INCLUDE_GUARD_ENGINE_REFEREE_HPP

#include <cmath>
#include <cstdint>
#include <memory>
#include <sstream>
#include <vector>

#include "engine_Agent.hpp"
#include "engine_GameState.hpp"
#include "util_General.hpp"

namespace engine {

class IAgentFactory {
 public:
  virtual std::unique_ptr<Agent> MakeAgent() const = 0;
};

struct Episode {
  struct Frame {
    GameState state;
    Move move;
    u_int player;
    u_int winner;
    uint64_t arid;

    Frame(GameState const& a_state, Move a_move, u_int a_player,
          uint64_t a_arid)
        : state(a_state),
          move(a_move),
          player(a_player),
          winner(2),
          arid(a_arid) {}
  };

  std::vector<Frame> episode;
};

class Referee {
 private:
  struct Player {
    Agent* a;
    std::istringstream in{};
    std::ostringstream out{};
  };

  struct Observer {
    void (*frame)(engine::GameState const& s, Move const& m, u_int player,
                  uint64_t arid, void* user_data);
    void (*end)(u_int winner, void* user_data);
    void* user_data;
  };

 public:
  static Episode CollectEpisode(IAgentFactory const& left,
                                IAgentFactory const& right) {
    auto left_agent = left.MakeAgent();
    auto right_agent = right.MakeAgent();

    Episode e{};
    e.episode.reserve(150);

    Observer o = {[](engine::GameState const& s, Move const& m, u_int player,
                     uint64_t arid, void* user_data) {
                    auto p_e = static_cast<Episode*>(user_data);
                    p_e->episode.emplace_back(s, m, player, arid);
                  },
                  [](u_int winner, void* user_data) {
                    auto p_e = static_cast<Episode*>(user_data);
                    for (auto& f : p_e->episode) {
                      f.winner = winner;
                    }
                  },
                  &e};

    PlayGame(*left_agent, *right_agent, &o);
    return e;
  }

  static u_int PlayGame(Agent& p0, Agent& p1,
                        Observer const* observer = nullptr) {
    std::array<Player, 2> players{Player{&p0}, Player{&p1}};

    uint64_t arid;
    auto g = GameState::RandomStart(arid);

    for (auto& player : players) {
      StartGame(player, arid);
    }

    while (!g.IsTerminal()) {
      if (g.IsWaiting(0) || g.IsWaiting(1)) {
        Turn(players[g.NextPlayer()], g, arid, observer);
      } else {
        Turn(players[0], players[1], g, arid, observer);
      }
    }

    u_int p0_score = g.GetScore(0) + g.GetSun(0) / 3;
    u_int p1_score = g.GetScore(1) + g.GetSun(1) / 3;
    u_int winner = 2;

    if (p0_score > p1_score) {
      winner = 0;
    } else if (p1_score > p0_score) {
      winner = 1;
    } else {
      auto p0_trees = util::popcnt(g.GetPlayerTrees(0));
      auto p1_trees = util::popcnt(g.GetPlayerTrees(1));

      if (p0_trees > p1_trees) {
        winner = 0;
      } else if (p1_trees > p0_trees) {
        winner = 1;
      }
    }

    if (observer) {
      observer->end(winner, observer->user_data);
    }

    return winner;
  }

 private:
  static void StartGame(Player& p, uint64_t const& arid) {
    std::ostringstream init;
    init << 37 << "\n";
    for (u_int i = 0; i < 37; ++i) {
      bool is_arid = (arid & GetTree(i));
      u_int richness = is_arid ? 0 : (RICHNESS[i] / 2 + 1);

      init << i << " " << richness;

      for (u_int n = 0; n < 6; ++n) {
        init << " " << static_cast<int>(NEIGHBOR_TABLE[i][n]);
      }

      init << "\n";
    }

    p.a->SetStreams(p.in, p.out);
    p.in.str(init.str());
    p.a->Init();
  }

  static void Turn(Player& p, GameState& g, uint64_t const& arid,
                   Observer const* observer) {
    SetInput(p, g, arid, g.NextPlayer());
    auto move = p.a->Turn();
    if (observer) {
      observer->frame(g, move, g.NextPlayer(), arid, observer->user_data);
    }
    ReadOutput(p, g, arid, g.NextPlayer());
  }

  static void Turn(Player& p0, Player& p1, GameState& g, uint64_t const& arid,
                   Observer const* observer) {
    SetInput(p0, g, arid, 0);
    SetInput(p1, g, arid, 1);
    auto p0_move = p0.a->Turn();
    auto p1_move = p1.a->Turn();

    if (observer) {
      observer->frame(g, p0_move, 0, arid, observer->user_data);
      observer->frame(g, p1_move, 1, arid, observer->user_data);
    }

    ReadOutput(p0, g, arid, 0);
    ReadOutput(p1, g, arid, 1);
  }

  static void SetInput(Player& p, GameState& g, uint64_t const& arid,
                       u_int player) {
    std::ostringstream turn_input;

    turn_input << static_cast<u_int>(g.GetDay()) << "\n";
    turn_input << static_cast<u_int>(g.GetNutrients()) << "\n";
    turn_input << static_cast<u_int>(g.GetSun(player)) << " "
               << static_cast<u_int>(g.GetScore(player)) << "\n";
    turn_input << static_cast<u_int>(g.GetSun(1 - player)) << " "
               << static_cast<u_int>(g.GetScore(1 - player)) << " "
               << (g.IsWaiting(1 - player) ? 1 : 0) << "\n";
    turn_input << util::popcnt(g.GetAllTrees()) << "\n";

    for (u_int s = 0; s < 4; ++s) {
      IterateTrees(g.GetTrees(s), [&](u_int offset) {
        turn_input << offset << " " << s << " "
                   << ((g.GetOwner(player) & GetTree(offset)) ? 1 : 0) << " "
                   << ((g.GetDormant() & GetTree(offset)) ? 1 : 0) << "\n";
      });
    }

    auto params = GameState::MoveFilterParams::Default();
    params.block_self_neighbor = false;
    auto moves = g.GetMoves(player, arid, params);
    turn_input << moves.size() << "\n";
    for (auto const& m : moves) {
      turn_input << m << "\n";
    }

    p.in.str(turn_input.str());
  }

  static void ReadOutput(Player& p, GameState& g, uint64_t const& arid,
                         u_int player) {
    std::istringstream turn_output(p.out.str());
    p.out.str("");
    p.out.clear();

    std::string action;
    turn_output >> action;

    Move m = Move::Invalid();
    if (action == "WAIT") {
      m = Move::Wait();
    } else if (action == "GROW") {
      u_int target;
      turn_output >> target;
      m = Move::Grow(static_cast<uint8_t>(target));
    } else if (action == "COMPLETE") {
      u_int target;
      turn_output >> target;
      m = Move::Complete(static_cast<uint8_t>(target));
    } else if (action == "SEED") {
      u_int target;
      u_int destination;
      turn_output >> target >> destination;
      m = Move::Seed(static_cast<uint8_t>(target),
                     static_cast<uint8_t>(destination));
    }

    ASSERT(m.IsValid());
    ASSERT(player == g.NextPlayer());
    g.Turn(player, m, arid);
  }
};
}  // namespace engine

#endif /* __INCLUDE_GUARD_ENGINE_REFEREE_HPP */