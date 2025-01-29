#ifndef __INCLUDE_GUARD_ENGINE_TABLES_HPP
#define __INCLUDE_GUARD_ENGINE_TABLES_HPP

#include <array>
#include <cmath>
#include <cstdint>

namespace engine {

static uint64_t constexpr N_TREES = 37u;
static uint64_t constexpr TREE_MASK = 0x1FFFFFFFFF;

static uint64_t constexpr GetTree(u_int offset) { return 1ull << offset; }

static int8_t constexpr NEIGHBOR_TABLE[N_TREES][6] = {
    {1, 2, 3, 4, 5, 6},       {7, 8, 2, 0, 6, 18},
    {8, 9, 10, 3, 0, 1},      {2, 10, 11, 12, 4, 0},
    {0, 3, 12, 13, 14, 5},    {6, 0, 4, 14, 15, 16},
    {18, 1, 0, 5, 16, 17},    {19, 20, 8, 1, 18, 36},
    {20, 21, 9, 2, 1, 7},     {21, 22, 23, 10, 2, 8},
    {9, 23, 24, 11, 3, 2},    {10, 24, 25, 26, 12, 3},
    {3, 11, 26, 27, 13, 4},   {4, 12, 27, 28, 29, 14},
    {5, 4, 13, 29, 30, 15},   {16, 5, 14, 30, 31, 32},
    {17, 6, 5, 15, 32, 33},   {35, 18, 6, 16, 33, 34},
    {36, 7, 1, 6, 17, 35},    {-1, -1, 20, 7, 36, -1},
    {-1, -1, 21, 8, 7, 19},   {-1, -1, 22, 9, 8, 20},
    {-1, -1, -1, 23, 9, 21},  {22, -1, -1, 24, 10, 9},
    {23, -1, -1, 25, 11, 10}, {24, -1, -1, -1, 26, 11},
    {11, 25, -1, -1, 27, 12}, {12, 26, -1, -1, 28, 13},
    {13, 27, -1, -1, -1, 29}, {14, 13, 28, -1, -1, 30},
    {15, 14, 29, -1, -1, 31}, {32, 15, 30, -1, -1, -1},
    {33, 16, 15, 31, -1, -1}, {34, 17, 16, 32, -1, -1},
    {-1, 35, 17, 33, -1, -1}, {-1, 36, 18, 17, 34, -1},
    {-1, 19, 7, 18, 35, -1},
};

static uint8_t constexpr OPPOSITE[N_TREES] = {
    0,  4,  5,  6,  1,  2,  3,  13, 14, 15, 16, 17, 18, 7,  8,  9,  10, 11, 12,
    28, 29, 30, 31, 32, 33, 34, 35, 36, 19, 20, 21, 22, 23, 24, 25, 26, 27};

inline auto constexpr MakeRichness() {
  std::array<uint8_t, N_TREES> table{};

  for (uint8_t o = 0u; o < N_TREES; ++o) {
    if (o <= 6) {
      table[o] = 4u;
    } else if (o <= 18) {
      table[o] = 2u;
    } else {
      table[o] = 0u;
    }
  }

  return table;
};

static auto constexpr RICHNESS = MakeRichness();

inline auto constexpr MakeShadowTable() {
  std::array<std::array<std::array<uint64_t, N_TREES>, 3u>, 6u> table{};

  for (uint8_t d = 0; d < 6; ++d) {
    for (uint8_t t = 0; t < 3; ++t) {
      for (int8_t o = 0; o < static_cast<int8_t>(N_TREES); ++o) {
        uint64_t shadow = 0;
        int8_t cursor = o;

        for (uint8_t l = 0; l < (t + 1); ++l) {
          if (NEIGHBOR_TABLE[cursor][d] != -1) {
            cursor = NEIGHBOR_TABLE[cursor][d];
            shadow |= GetTree(cursor);
          } else {
            break;
          }
        }

        table[d][t][o] = shadow;
      }
    }
  }

  return table;
}

static auto constexpr SHADOW_TABLE = MakeShadowTable();

inline auto constexpr MakeSeedBlock() {
  std::array<uint64_t, N_TREES> table{};

  for (u_int o = 0u; o < N_TREES; ++o) {
    uint64_t block = (1ull << o);

    for (u_int n = 0u; n < 6u; ++n) {
      if (NEIGHBOR_TABLE[o][n] != static_cast<int8_t>(-1)) {
        block |= GetTree(NEIGHBOR_TABLE[o][n]);
      }
    }

    table[o] = block;
  }

  return table;
}

static auto constexpr SEED_BLOCK = MakeSeedBlock();

inline auto constexpr MakeSeedDestinationTable() {
  std::array<std::array<uint64_t, N_TREES>, 3> table{};

  for (uint8_t size = 1; size <= 3; ++size) {
    for (u_int target = 0u; target < N_TREES; ++target) {
      uint64_t destinations = GetTree(target);

      /* Iteratively expand from the neighbors. */
      for (uint8_t depth = 0; depth < size; ++depth) {
        uint64_t new_destinations = 0u;
        for (uint8_t offset = 0; offset < N_TREES; ++offset) {
          if (destinations & GetTree(offset)) {
            new_destinations |= SEED_BLOCK[offset];
          }
        }
        destinations = new_destinations;
      }

      /* The tree location is never valid */
      destinations &= ~GetTree(target);

      table[size - 1][target] = destinations;
    }
  }

  return table;
}

static auto constexpr SEED_DESTINATIONS = MakeSeedDestinationTable();

}  // namespace engine

#endif /* __INCLUDE_GUARD_ENGINE_TABLES_HPP */