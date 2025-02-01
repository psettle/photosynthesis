#pragma once
#include "engine_GameState.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>
#include <vector>

namespace engine {

void GameState::ReadBoard(std::istream& input, uint64_t& arid) {
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

GameState GameState::RandomStart(uint64_t& arid) {
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

GameState GameState::FromStream(std::istream& input, TimeStamp& start,
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

void GameState::GetMoves(u_int player,
                         void (*callback)(Move const& m, void* data),
                         void* data, uint64_t const& arid,
                         MoveFilterParams const params) const {
  if (IsTerminal()) {
    return;
  }

  /* Can always wait. */
  callback(Move::Wait(), data);

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
                     callback(Move::Grow(static_cast<uint8_t>(offset)), data);
                   });
    }
  }

  if (params.can_complete) {
    /* Get complete moves */
    if (sun >= 4) {
      IterateTrees(
          GetPlayerTrees(player, 3) & ~GetDormant(), [&](u_int offset) {
            callback(Move::Complete(static_cast<uint8_t>(offset)), data);
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
              uint64_t dest = SEED_DESTINATIONS[s - 1][target] & ~block & ~arid;
              IterateTrees(dest, [&](u_int destination) {
                callback(Move::Seed(static_cast<uint8_t>(target),
                                    static_cast<uint8_t>(destination)),
                         data);
              });
            });
      }
    }
  }
}

void GameState::Turn(u_int player, Move const& m, uint64_t const& arid) {
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

void GameState::PlaceTree(u_int offset, u_int size, u_int player,
                          bool dormant) {
  auto s = GetTree(offset);
  trees_[size] |= s;
  owner_[player] |= s;
  if (dormant) {
    dormant_ |= s;
  }
}

void GameState::GrowSeed(u_int source_offset, u_int dest_offset, u_int player,
                         u_int cost) {
  auto seed = GetTree(dest_offset);
  trees_[0] |= seed;
  owner_[player] |= seed;
  dormant_ |= seed;

  auto source = GetTree(source_offset);
  dormant_ |= source;

  RemoveSun(cost, player);
}

void GameState::CompleteTree(u_int offset, u_int player, u_int cost,
                             u_int reward) {
  auto tree = GetTree(offset);
  trees_[3] &= ~tree;
  owner_[player] &= ~tree;

  RemoveSun(cost, player);
  AddScore(reward, player);

  int nutrients = std::max(0, static_cast<int>(GetNutrients() - 1));
  SetNutrients(static_cast<uint8_t>(nutrients));
}

void GameState::EndDay() {
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

}  // namespace engine