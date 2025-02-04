#ifndef __INCLUDE_GUARD_AGENT_MCTS_HPP
#define __INCLUDE_GUARD_AGENT_MCTS_HPP

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <vector>

#include "engine_Agent.hpp"
#include "engine_GameState.hpp"
#include "engine_Move.hpp"
#include "util_General.hpp"
#include "util_TimeStamp.hpp"

namespace agent {
class Mcts : public engine::Agent {
 public:
  using GameState = engine::GameState;
  using Move = engine::Move;
  using TimeStamp = util::TimeStamp;

  struct Node {
    GameState gs;
    std::vector<Node> children;
    std::vector<Move> unexplored;
    uint32_t n_rollouts{};
    float score{};
    Move preceeding;

    Node(GameState const& a_gs, std::vector<Move>&& a_unexplored,
         Move const& a_preceeding = Move::Invalid())
        : gs(a_gs),
          unexplored(std::move(a_unexplored)),
          preceeding(a_preceeding) {}
  };

  virtual float Heuristic(GameState const& gamestate) {
    return Simulate(gamestate);
  }

  void Init() override {
    Agent::Init();
    ResetHistory();
    first_turn_ = true;
  }

  virtual bool CheckLimit(TimeStamp const& start, size_t n_rollouts,
                          bool first_turn) {
    return start.Since() >= (first_turn ? 0.995 : 0.095);
  }

  void ResetHistory() { root_.reset(); }

  Move ChooseMove(GameState const& state, TimeStamp const& start) override {
    if (!TrackActualAction(state)) {
      root_.emplace(state, GetTreeMoves(state));
    }

    auto& root = root_.value();

    std::vector<Node*> expand_path;
    while (root.n_rollouts <= 0xFFFFFE) {
      expand_path.clear();
      expand_path.push_back(&root);
      Select(expand_path, true);

      if (!expand_path.back()->unexplored.empty()) {
        Expand(expand_path);
      }

      float score = Heuristic(expand_path.back()->gs);

      Backup(expand_path, score);

      if (CheckLimit(start, root.n_rollouts, first_turn_)) {
        break;
      }
    }
    if (first_turn_) {
      // std::cerr << "R: " << root.n_rollouts << std::endl;
    }

    first_turn_ = false;

    ASSERT(!root.children.empty());

    auto best = util::MaxElement(root.children.begin(), root.children.end(),
                                 [](Node const& n) {
                                   // std::cerr << (n.score / n.n_rollouts) << "
                                   // " << n.n_rollouts << " "
                                   //           << n.preceeding << std::endl;
                                   ASSERT(n.n_rollouts > 0);
                                   return n.score / n.n_rollouts;
                                 });

    Node local = std::move(*best);
    root_ = std::move(local);
    // std::cerr << "*" << root_.value().preceeding << "\n" << std::endl;
    return root_.value().preceeding;
  }

 private:
  std::optional<Node> root_;
  bool first_turn_{true};

  bool TrackActualAction(GameState const& gs) {
    if (!root_) {
      return false;
    }

    auto& root = root_.value();

    if (root.gs.NextPlayer() == 0u) {
      if (!(root.gs == gs)) {
        std::cerr << static_cast<uint16_t>(root.gs.GetDay()) << " "
                  << static_cast<uint16_t>(gs.GetDay()) << std::endl;
        std::cerr << static_cast<uint16_t>(root.gs.GetScore(0)) << " "
                  << static_cast<uint16_t>(gs.GetScore(0)) << std::endl;
        std::cerr << static_cast<uint16_t>(root.gs.GetScore(11)) << " "
                  << static_cast<uint16_t>(gs.GetScore(1)) << std::endl;
        std::cerr << static_cast<uint16_t>(root.gs.GetSun(0)) << " "
                  << static_cast<uint16_t>(gs.GetSun(0)) << std::endl;
        std::cerr << static_cast<uint16_t>(root.gs.GetSun(1)) << " "
                  << static_cast<uint16_t>(gs.GetSun(1)) << std::endl;
      }
      ASSERT(root.gs == gs);
      /* Opponent is waiting so we predicted the state internally. */
      // std::cerr << "Op is waiting, reused " << root.n_rollouts << " trials"
      //           << std::endl;
      return true;
    }

    auto new_root =
        std::find_if(root.children.begin(), root.children.end(),
                     [&](Node const& child) { return child.gs == gs; });
    if (new_root == root.children.end()) {
      /* Opponent took a move we never explored, or multiple moves while we
       * waited. */
      return false;
    }

    /* Found the opponent's move, preserve the node. */
    // std::cerr << "Op did: " << new_root->preceeding << " reused "
    //           << new_root->n_rollouts << " trials" << std::endl;
    Node local = std::move(*new_root);
    root_ = std::move(local);
    return true;
  }

  void Select(std::vector<Node*>& path, bool is_maximizing) {
    Node& back = *path.back();
    if (!back.unexplored.empty()) {
      /* Unexplored actions on this path, we should explore them before going
       * deeper. */
      return;
    }

    if (back.children.empty()) {
      /* Terminal node (everything explored, no children), can't grow. */
      ASSERT(back.gs.IsTerminal());
      return;
    }

    ASSERT(back.n_rollouts > 0);

    float factor = is_maximizing ? 1.0 : -1.0;

    auto max = util::MaxElement(
        back.children.begin(), back.children.end(), [&](Node const& child) {
          ASSERT(child.n_rollouts > 0);

          float value =
              factor * child.score / child.n_rollouts +
              std::sqrt(2.0f * std::log(back.n_rollouts) / child.n_rollouts);
          return value;
        });

    /* Grow path and keep trying to find something to expand. */
    path.push_back(&*max);
    Select(path, !is_maximizing);
  }

  void Expand(std::vector<Node*>& path) {
    auto& unexplored = path.back()->unexplored;
    auto it = unexplored.begin() + Rand() % unexplored.size();
    auto move = *it;
    unexplored.erase(it);

    GameState next(path.back()->gs);
    next.Turn(next.NextPlayer(), move, GetArid());
    path.back()->children.emplace_back(next, GetTreeMoves(next), move);
    path.emplace_back(&path.back()->children.back());
  }

  std::vector<Move> GetRawMoves(GameState const& g) const {
    return g.GetMoves(g.NextPlayer(), GetArid());
  }

  std::vector<Move> GetTreeMoves(GameState const& g) const {
    auto params = GameState::MoveFilterParams::Default();
    params.can_seed = g.GetNumTrees(g.NextPlayer(), 0) == 0;

    uint8_t day = g.GetDay();

    if (day < 11) {
      params.can_complete = false;
    }

    std::vector<Move> moves;

    bool can_seed = false;
    auto filter = [&](Move const& m) {
      if (m.GetType() == Move::Type::kWait) {
        return;
      }
      if (m.GetType() == Move::Type::kSeed) {
        can_seed = true;
      }
      moves.emplace_back(m);
    };

    g.GetMoves(g.NextPlayer(), filter, GetArid(), params);

    if ((!can_seed || day > 21) && !g.IsTerminal()) {
      moves.emplace_back(Move::Wait());
    }

    return moves;
  }

  std::vector<Move> GetRolloutMoves(GameState const& g) const {
    auto params = GameState::MoveFilterParams::Default();
    params.can_seed = g.GetNumTrees(g.NextPlayer(), 0) == 0;

    uint8_t day = g.GetDay();

    if (day < 11) {
      params.can_complete = false;
    } else if (day < 22) {
      params.can_complete = g.GetNumTrees(g.NextPlayer(), 3) >= 4;
    }

    if (day > 19) {
      params.can_grow = false;
    }

    std::vector<Move> moves;

    bool can_complete = false;
    bool can_grow = false;
    bool can_seed = false;
    auto filter = [&](Move const& m) {
      switch (m.GetType()) {
        case Move::Type::kWait:
          return;
        case Move::Type::kComplete:
          can_complete = true;
          break;
        case Move::Type::kGrow:
          can_grow = true;
          break;
        case Move::Type::kSeed:
          can_seed = true;
          break;
      }
      moves.emplace_back(m);
    };

    g.GetMoves(g.NextPlayer(), filter, GetArid(), params);

    if (can_grow && can_complete) {
      /* Don't grow if we can complete. */
      moves.erase(std::remove_if(moves.begin(), moves.end(),
                                 [](Move const& m) {
                                   return m.GetType() == Move::Type::kGrow;
                                 }),
                  moves.end());
      can_grow = false;
    }

    if (can_seed && (can_grow || can_complete)) {
      /* Don't seed if we can grow or complete. */
      moves.erase(std::remove_if(moves.begin(), moves.end(),
                                 [](Move const& m) {
                                   return m.GetType() == Move::Type::kSeed;
                                 }),
                  moves.end());
      can_seed = false;
    }

    if (!g.IsTerminal()) {
      moves.emplace_back(Move::Wait());
    }

    return moves;
  }

  float Simulate(GameState const& state) {
    GameState g = state;
    while (!g.IsTerminal()) {
      auto moves = GetRolloutMoves(g);
      g.Turn(g.NextPlayer(), moves[Rand() % moves.size()], GetArid());
    }

    return Score(g);
  }

  static float Score(GameState const& game) {
    float p0_score = game.GetScore(0) + game.GetSun(0) / 3;
    float p1_score = game.GetScore(1) + game.GetSun(1) / 3;

    if (p0_score > p1_score) {
      float diff = p0_score - p1_score;

      if (diff > 5) {
        return 1.0f + (diff - 5) * 0.001f;
      } else {
        return 0.5f + 0.5f * diff / 5;
      }
    } else if (p0_score < p1_score) {
      float diff = p1_score - p0_score;

      if (diff > 5) {
        return -1.0f - (diff - 5) * 0.001f;
      } else {
        return -0.5f - 0.5f * diff / 5;
      }
    } else {
      return 0.0f;
    }
  }

  static void Backup(std::vector<Node*> const& path, float score) {
    for (auto it = path.rbegin(); it != path.rend(); ++it) {
      (*it)->n_rollouts++;
      (*it)->score += score;
    }
  }
};
}  // namespace agent

#endif /* __INCLUDE_GUARD_AGENT_MCTS_HPP */
