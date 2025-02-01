
#ifndef __INCLUDE_GUARD_ENGINE_GAMESTATE_HPP
#define __INCLUDE_GUARD_ENGINE_GAMESTATE_HPP

#include <array>
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

  static void ReadBoard(std::istream& input, uint64_t& arid);

  static GameState FromStream(std::istream& input, TimeStamp& start,
                              uint64_t const& arid);

  static GameState RandomStart(uint64_t& arid);

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
    auto local = std::move(callable);
    GetMoves(
        player,
        [](Move const& m, void* data) {
          Callable& p_callable = *static_cast<decltype(local)*>(data);
          (p_callable)(m);
        },
        &local, arid, params);
  }

  void Turn(u_int player, Move const& m, uint64_t const& arid);

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
  void GetMoves(u_int player, void (*callback)(Move const& m, void* data),
                void* data, uint64_t const& arid,
                MoveFilterParams const params) const;

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

  void PlaceTree(u_int offset, u_int size, u_int player, bool dormant);

  void GrowSeed(u_int source_offset, u_int dest_offset, u_int player,
                u_int cost);

  void CompleteTree(u_int offset, u_int player, u_int cost, u_int reward);

  void EndDay();

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