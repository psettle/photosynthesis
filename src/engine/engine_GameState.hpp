
#ifndef __INCLUDE_GUARD_ENGINE_GAMESTATE_HPP
#define __INCLUDE_GUARD_ENGINE_GAMESTATE_HPP

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <vector>

#include "engine_Move.hpp"
#include "engine_Tables.hpp"
#include "util_General.hpp"
#include "util_TimeStamp.hpp"

namespace engine {
class GameState {
 public:
  using TimeStamp = util::TimeStamp;

  static void ReadBoard(std::istream& input, uint64_t& arid) {
    arid = 0u;
    size_t n_tiles;
    input >> n_tiles;
    input.ignore();

    for (u_int t = 0u; t < n_tiles; ++t) {
      uint16_t index, richness;
      std::array<int16_t, 6> neighbors;
      input >> index >> richness;
      for (auto& n : neighbors) {
        input >> n;
      }
      input.ignore();

      if (richness == 0) {
        arid |= GetTree(index);
      }
    }
  }

  static GameState FromStream(std::istream& input, TimeStamp& start,
                              uint64_t const& arid) {
    GameState g;

    uint16_t day;
    input >> day;
    g.SetDay(static_cast<uint8_t>(day));

    start = TimeStamp();

    input.ignore();

    int16_t nutrients;
    input >> nutrients;
    input.ignore();

    g.SetNutrients(static_cast<uint8_t>(nutrients));

    uint16_t my_sun_points;
    uint16_t my_game_points;
    input >> my_sun_points >> my_game_points;
    input.ignore();

    g.SetSun(static_cast<uint8_t>(my_sun_points), 0);
    g.SetScore(static_cast<uint8_t>(my_game_points), 0);

    uint16_t op_sun_points;
    uint16_t op_game_points;
    bool op_is_waiting;
    input >> op_sun_points >> op_game_points >> op_is_waiting;
    input.ignore();
    if (op_is_waiting) {
      g.SetWaiting(1);
    }

    g.SetSun(static_cast<uint8_t>(op_sun_points), 1);
    g.SetScore(static_cast<uint8_t>(op_game_points), 1);

    size_t n_trees;
    input >> n_trees;
    input.ignore();

    for (size_t i = 0; i < n_trees; ++i) {
      uint16_t index;
      uint16_t size;
      uint16_t player;
      bool dormant;
      input >> index >> size >> player >> dormant;
      input.ignore();

      g.PlaceTree(index, size, 1 - player, dormant);
    }
    size_t n_actions;
    input >> n_actions;

    input.ignore();
    for (size_t i = 0; i < n_actions; ++i) {
      std::string action;
      std::getline(input, action);
      // std::cerr << action << std::endl;
    }

    if constexpr (util::ENABLE_ASSERTS) {
      MoveFilterParams params = MoveFilterParams::Default();
      params.block_self_neighbor = false;
      size_t expected_actions = 0;
      g.GetMoves(
          0,
          [&](Move const& m) {
            expected_actions++;
            // std::cerr << m << std::endl;
          },
          arid, params);
      ASSERT(n_actions == expected_actions);
    }

    return g;
  }

  static GameState RandomStart(uint64_t& arid) {
    GameState g;

    arid = 0u;

    std::random_device rand_device;
    std::default_random_engine rand_engine(rand_device());
    std::uniform_int_distribution<int> uniform_dist(0, RAND_MAX);
    auto rand_func = [&]() -> int { return uniform_dist(rand_engine); };

    int n_desired_holes = rand_func() % 11;
    int n_holes = 0;
    std::vector<u_int> options;
    for (u_int i = 19; i < N_TREES; ++i) {
      options.push_back(i);
    }
    while (n_holes < n_desired_holes - 1) {
      u_int candidate = rand_func() % N_TREES;

      if (GetTree(candidate) & arid) {
        /* Already arid. */
        continue;
      }

      arid |= GetTree(candidate);
      n_holes++;
      options.erase(std::remove(options.begin(), options.end(), candidate),
                    options.end());
      if (candidate > 0) {
        arid |= GetTree(OPPOSITE[candidate]);
        n_holes++;
        options.erase(
            std::remove(options.begin(), options.end(), OPPOSITE[candidate]),
            options.end());
      }
    }

    auto Distance = [](u_int l, u_int r) -> int {
      if (l == 35 && r == 19) {
        return 1;
      }
      if (l == 36 && r == 19) {
        return 0;
      }
      if (l == 36 && r == 20) {
        return 1;
      }

      if (l == r) {
        return -1;
      }

      if (l > r) {
        return (l - r) - 1;
      } else {
        return (r - l) - 1;
      }
    };

    ASSERT(!options.empty());

    int n_trees = 0;
    while (n_trees < 4) {
      auto new_options = options;
      u_int first = new_options[rand_func() % new_options.size()];

      new_options.erase(
          std::remove(new_options.begin(), new_options.end(), first),
          new_options.end());
      new_options.erase(
          std::remove(new_options.begin(), new_options.end(), OPPOSITE[first]),
          new_options.end());

      u_int second = new_options[rand_func() % new_options.size()];
      u_int third = OPPOSITE[first];
      u_int fourth = OPPOSITE[second];

      std::array<u_int, 4> candidates = {first, second, third, fourth};

      bool fail = false;
      for (u_int c = 0; c < 4 && !fail; ++c) {
        for (u_int o = 0; o < 4 && !fail; ++o) {
          if (o == c) {
            continue;
          }

          if (Distance(candidates[c], candidates[o]) < 2) {
            fail = true;
          }
        }
      }

      if (!fail) {
        n_trees = 4;
        g.PlaceSapling(candidates[0], 0);
        g.PlaceSapling(candidates[1], 0);
        g.PlaceSapling(candidates[2], 1);
        g.PlaceSapling(candidates[3], 1);
      }
    }

    g.SetNutrients(20);
    return g;
  }

  GameState() { StoreMove(Move::Invalid()); }

  struct MoveFilterParams {
    bool block_self_neighbor;
    bool can_seed;
    bool can_grow;
    bool can_complete;

    static MoveFilterParams Default() {
      return MoveFilterParams{true, true, true, true};
    }
  };

  std::vector<Move> GetMoves(
      u_int player, uint64_t const& arid,
      MoveFilterParams const params = MoveFilterParams::Default()) const {
    std::vector<Move> moves;
    GetMoves(
        player, [&](Move const& m) { moves.emplace_back(m); }, arid, params);
    return moves;
  }

  template <class Callable>
  void GetMoves(
      u_int player, Callable&& callable, uint64_t const& arid,
      MoveFilterParams const params = MoveFilterParams::Default()) const {
    if (IsTerminal()) {
      return;
    }

    /* Can always wait. */
    callable(Move::Wait());

    uint8_t sun = GetSun(player);

    /* Get grow moves */
    if (params.can_grow) {
      for (u_int s = 0; s < 3; ++s) {
        u_int n_trees = GetNumTrees(player, s + 1);
        if (sun < n_trees + GetTreeFixedCost(s + 1)) {
          continue;
        }

        IterateTrees(GetPlayerTrees(player, s) & ~GetDormant(),
                     [&](u_int offset) {
                       callable(Move::Grow(static_cast<uint8_t>(offset)));
                     });
      }
    }

    if (params.can_complete) {
      /* Get complete moves */
      if (sun >= 4) {
        IterateTrees(GetPlayerTrees(player, 3) & ~GetDormant(),
                     [&](u_int offset) {
                       callable(Move::Complete(static_cast<uint8_t>(offset)));
                     });
      }
    }

    if (params.can_seed) {
      /* Get seed moves */
      if (sun >= GetNumTrees(player, 0)) {
        uint64_t block = GetAllTrees();

        if (params.block_self_neighbor) {
          IterateTrees(GetPlayerTrees(player),
                       [&](u_int offset) { block |= SEED_BLOCK[offset]; });
        }

        for (u_int s = 1; s <= 3; ++s) {
          if (params.block_self_neighbor && s == 1) {
            continue;
          }

          IterateTrees(
              GetPlayerTrees(player, s) & ~GetDormant(), [&](u_int target) {
                uint64_t dest =
                    SEED_DESTINATIONS[s - 1][target] & ~block & ~arid;
                IterateTrees(dest, [&](u_int destination) {
                  callable(Move::Seed(static_cast<uint8_t>(target),
                                      static_cast<uint8_t>(destination)));
                });
              });
        }
      }
    }
  }

  void Turn(u_int player, Move const& m, uint64_t const& arid) {
    ASSERT(player == NextPlayer());
    ASSERT(GetDay() <= 23);

    auto last_move = GetMove();
    if (last_move.IsValid()) {
      ASSERT(player == 1);
      ClearMove();

      if (last_move.GetType() == m.GetType()) {
        switch (last_move.GetType()) {
          case Move::Type::kSeed:
            if (last_move.GetDestination() == m.GetDestination()) {
              ASSERT(GetSize(m.GetTarget()) > 0);
              ASSERT(GetTree(m.GetTarget()) & owner_[player]);
              ASSERT(GetTree(m.GetTarget()) & ~dormant_);
              ASSERT(GetSun(player) >= GetNumTrees(player, 0));
              ASSERT(GetSize(m.GetDestination()) == 0);
              ASSERT(GetTree(m.GetDestination()) & owner_[0]);
              ASSERT(GetTree(m.GetDestination()) & ~arid);
              ASSERT(
                  SEED_DESTINATIONS[GetSize(m.GetTarget()) - 1][m.GetTarget()] &
                  GetTree(m.GetDestination()));

              /* Both seeded same place, remove seed and refund sun. */
              uint64_t t = GetTree(last_move.GetDestination());
              trees_[0] &= ~t;
              dormant_ &= ~t;
              owner_[0] &= ~t;
              dormant_ |= GetTree(m.GetTarget());
              AddSun(GetLastSeedCost(), 0);
              return;
            }
            break;
          default:
            break;
        }
      }
    } else if (!IsWaiting(1) && player == 0u) {
      StoreMove(m);
      SetLastNutrients(GetNutrients());
    }

    switch (m.GetType()) {
      case Move::Type::kWait:
        SetWaiting(player);
        if (IsWaiting(1 - player)) {
          /* Both waited. */
          EndDay();
          return;
        }
        break;
      case Move::Type::kSeed: {
        u_int cost = GetNumTrees(player, 0);

        ASSERT(GetSize(m.GetTarget()) > 0);
        ASSERT(GetTree(m.GetTarget()) & owner_[player]);
        ASSERT(GetTree(m.GetTarget()) & ~dormant_);
        ASSERT(GetSun(player) >= cost);
        ASSERT(GetTree(m.GetDestination()) & ~GetAllTrees());
        ASSERT(GetTree(m.GetDestination()) & ~arid);
        ASSERT(GetTree(m.GetDestination()) & ~owner_[0]);
        ASSERT(GetTree(m.GetDestination()) & ~owner_[1]);
        ASSERT(GetSize(m.GetDestination()) == -1);
        ASSERT(SEED_DESTINATIONS[GetSize(m.GetTarget()) - 1][m.GetTarget()] &
               GetTree(m.GetDestination()));

        GrowSeed(m.GetTarget(), m.GetDestination(), player, cost);
        if (!last_move.IsValid()) {
          SetLastSeedCost(cost);
        }
      } break;
      case Move::Type::kGrow: {
        ASSERT(GetSize(m.GetTarget()) >= 0);

        u_int s = GetSize(m.GetTarget());
        u_int cost = GetNumTrees(player, s + 1) + GetTreeFixedCost(s + 1);

        ASSERT(GetTree(m.GetTarget()) & owner_[player]);
        ASSERT(GetTree(m.GetTarget()) & ~dormant_);
        ASSERT(GetSun(player) >= cost);

        GrowTree(m.GetTarget(), player, s, cost);
      } break;
      case Move::Type::kComplete: {
        ASSERT(GetSize(m.GetTarget()) == 3);
        ASSERT(GetTree(m.GetTarget()) & owner_[player]);
        ASSERT(GetTree(m.GetTarget()) & ~dormant_);
        ASSERT(GetSun(player) >= 4);
        ASSERT(GetNutrients() <= 20);

        uint8_t nutrients;

        if (!last_move.IsValid()) {
          nutrients = GetNutrients();
        } else {
          nutrients = GetLastNutrients();
        }

        CompleteTree(m.GetTarget(), player, 4u,
                     nutrients + RICHNESS[m.GetTarget()]);
      } break;
    }
  }

  u_int NextPlayer() const {
    if (IsWaiting(0)) {
      return 1;
    }

    if (IsWaiting(1)) {
      return 0;
    }

    return GetMove().IsValid() ? 1u : 0u;
  }

  bool IsTerminal() const { return GetDay() == 24; }

  u_int GetNumTrees(u_int player, u_int size) const {
    return util::popcnt(GetPlayerTrees(player, size));
  }

  uint8_t GetDay() const { return GetByte(56u, trees_[0]); }

  uint8_t GetScore(u_int player) const {
    return GetByte(48u + 8u * (player > 0 ? 1 : 0), trees_[1]);
  }
  uint8_t GetSun(u_int player) const {
    return GetByte(48u + 8u * (player > 0 ? 1 : 0), trees_[2]);
  }

  bool operator==(GameState const& other) const {
    for (size_t s = 0; s < 4; ++s) {
      if (GetTrees(s) != other.GetTrees(s)) {
        return false;
      }
    }

    if (GetDormant() != other.GetDormant()) {
      return false;
    }

    for (size_t p = 0; p < 2; ++p) {
      if ((owner_[p] & TREE_MASK) != (other.owner_[p] & TREE_MASK)) {
        return false;
      }

      if (GetScore(p) != other.GetScore(p)) {
        return false;
      }

      if (GetSun(p) != other.GetSun(p)) {
        return false;
      }

      if (IsWaiting(p) != other.IsWaiting(p)) {
        return false;
      }
    }

    if (GetNutrients() != other.GetNutrients()) {
      return false;
    }

    return true;
  }

  bool IsWaiting(u_int player) const { return owner_[player] & (1ull << 63); }
  uint8_t GetNutrients() const { return GetByte(48u, trees_[0]); }

  template <class Callable>
  static void IterateTrees(uint64_t trees, Callable&& callable) {
    while (trees != 0) {
      int offset = __builtin_ctzll(trees);
      callable(static_cast<u_int>(offset));
      trees &= ~(1ull << offset);
    }
  }

  uint64_t GetTrees(uint size) const { return TREE_MASK & trees_[size]; }
  uint64_t GetAllTrees() const {
    return (trees_[0] | trees_[1] | trees_[2] | trees_[3]) & TREE_MASK;
  }

  uint64_t GetDormant() const { return dormant_; }

  uint64_t GetOwner(u_int player) const { return owner_[player]; }

  uint64_t GetPlayerTrees(u_int player) const {
    return GetAllTrees() & owner_[player];
  }

 private:
  void SetWaiting(u_int player) { owner_[player] |= (1ull << 63); }

  void ClearWaiting() {
    owner_[0] &= ~(1ull << 63);
    owner_[1] &= ~(1ull << 63);
  }

  void SetLastSeedCost(uint8_t seed_cost) {
    SetByte(seed_cost, 40u, trees_[3]);
  }
  uint8_t GetLastSeedCost() const { return GetByte(40u, trees_[3]); }

  void SetLastNutrients(uint8_t nutrients) {
    SetByte(nutrients, 40u, trees_[2]);
  }
  uint8_t GetLastNutrients() const { return GetByte(40u, trees_[2]); }

  static u_int GetTreeFixedCost(u_int size) { return (1u << size) - 1u; }

  uint64_t GetPlayerTrees(u_int player, u_int size) const {
    return GetTrees(size) & owner_[player];
  }

  void GrowTree(u_int offset, u_int player, u_int size, u_int cost) {
    auto tree = GetTree(offset);
    trees_[size] &= ~tree;
    trees_[size + 1] |= tree;
    dormant_ |= tree;

    RemoveSun(cost, player);
  }

  int GetSize(u_int offset) {
    auto t = GetTree(offset);
    for (u_int s = 0u; s < 4; ++s) {
      if (trees_[s] & t) {
        return s;
      }
    }

    return -1;
  }

  void PlaceSapling(u_int offset, u_int player) {
    PlaceTree(offset, 1, player, false);
  }

  void PlaceTree(u_int offset, u_int size, u_int player, bool dormant) {
    auto s = GetTree(offset);
    trees_[size] |= s;
    owner_[player] |= s;
    if (dormant) {
      dormant_ |= s;
    }
  }

  void GrowSeed(u_int source_offset, u_int dest_offset, u_int player,
                u_int cost) {
    auto seed = GetTree(dest_offset);
    trees_[0] |= seed;
    owner_[player] |= seed;
    dormant_ |= seed;

    auto source = GetTree(source_offset);
    dormant_ |= source;

    RemoveSun(cost, player);
  }

  void CompleteTree(u_int offset, u_int player, u_int cost, u_int reward) {
    auto tree = GetTree(offset);
    trees_[3] &= ~tree;
    owner_[player] &= ~tree;

    RemoveSun(cost, player);
    AddScore(reward, player);

    int nutrients = std::max(0, static_cast<int>(GetNutrients() - 1));
    SetNutrients(static_cast<uint8_t>(nutrients));
  }

  void EndDay() {
    uint8_t day = GetDay() + 1u;
    ClearWaiting();

    if (GetDay() < 23) {
      uint8_t direction = day % 6;
      u_int sun_points[2]{};
      uint64_t shadow_map = 0;

      for (u_int s = 3; s > 0; s--) {
        IterateTrees(GetTrees(s), [&](u_int offset) {
          shadow_map |= SHADOW_TABLE[direction][s - 1][offset];
        });

        for (u_int player = 0u; player < 2; ++player) {
          uint64_t valid_trees = GetPlayerTrees(player, s) & ~shadow_map;
          sun_points[player] += util::popcnt(valid_trees) * s;
        }
      }

      for (u_int player = 0u; player < 2; ++player) {
        AddSun(sun_points[player], player);
      }

      dormant_ = 0u;
    }

    SetDay(day);
  }

  void SetDay(uint8_t day) { SetByte(day, 56u, trees_[0]); }
  void SetNutrients(uint8_t nutrients) { SetByte(nutrients, 48u, trees_[0]); }
  void SetScore(uint8_t score, u_int player) {
    SetByte(score, 48u + 8u * (player > 0 ? 1 : 0), trees_[1]);
  }
  void SetSun(uint8_t sun, u_int player) {
    SetByte(sun, 48u + 8u * (player > 0 ? 1 : 0), trees_[2]);
  }

  void RemoveSun(u_int amount, u_int player) {
    SetSun(GetSun(player) - static_cast<uint8_t>(amount), player);
  }
  void AddSun(u_int amount, u_int player) {
    SetSun(GetSun(player) + static_cast<uint8_t>(amount), player);
  }
  void AddScore(u_int amount, u_int player) {
    SetScore(GetScore(player) + static_cast<uint8_t>(amount), player);
  }

  static void SetByte(uint8_t byte, u_int offset, uint64_t& buffer) {
    reinterpret_cast<uint8_t*>(&buffer)[offset / 8u] = byte;
  }

  static uint8_t GetByte(u_int offset, uint64_t buffer) {
    return reinterpret_cast<uint8_t*>(&buffer)[offset / 8u];
  }

  void StoreMove(Move const& m) {
    auto dest = reinterpret_cast<uint16_t*>(&trees_[3]) + 3;
    *dest = Move::ToInt(m);
  }

  void ClearMove() { StoreMove(Move::Invalid()); }

  Move GetMove() const {
    auto source = reinterpret_cast<uint16_t const*>(&trees_[3]) + 3;
    return Move::FromInt(*source);
  }

  std::array<uint64_t, 4> trees_{};
  std::array<uint64_t, 2> owner_{};
  uint64_t dormant_{};
};
}  // namespace engine

#endif /* __INCLUDE_GUARD_ENGINE_GAMESTATE_HPP */