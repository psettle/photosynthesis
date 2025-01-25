#pragma GCC optimize "O3,omit-frame-pointer,inline"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <optional>
#include <vector>

using Time = std::chrono::high_resolution_clock;

class TimeStamp {
 public:
  TimeStamp() : t_(Time::now()) {}

  double operator-(TimeStamp const& other) const {
    auto diff = t_ - other.t_;
    return diff.count() * 0.000000001;
  }

  double Since() const {
    TimeStamp now;
    return now - *this;
  }

 private:
  Time::time_point t_;
};

template <class It, class Evaluate>
It MaxElement(It begin, It end, Evaluate&& evaluate) {
  auto best = end;
  float best_score = -100000.0f;

  for (auto it = begin; it != end; ++it) {
    float score = evaluate(*it);
    if (score > best_score) {
      best = it;
      best_score = score;
    }
  }

  return best;
}

template <class T>
class TightVector {
 private:
  using Storage = std::aligned_storage_t<sizeof(T), alignof(T)>;

 public:
  TightVector()
      : storage_(std::make_unique<Storage[]>(8)), size_(0), capacity_(0) {}

  ~TightVector() { clear(); }

  void clear() {
    while (size_ > 0) {
      pop_back();
    }
  }

  uint16_t size() const { return size_; }

  void emplace_back(T&& t) {
    if (size_ >= capacity_) {
      Reallocate(2 * capacity_);
    }

    new (Get(storage_.get(), size_)) T(std::move(t));
    size_++;
  }

  template <typename... Args>
  void emplace_back(Args&&... args) {
    if (size_ >= capacity_) {
      Reallocate(2 * capacity_);
    }

    new (Get(storage_.get(), size_)) T(std::forward(args)...);
    size_++;
  }

  void pop_back() { Get(storage_.get(), --size_)->~T(); }

  T const& operator[](uint16_t i) const { return *Get(storage_.get(), i); }

  T& operator[](uint16_t i) { return *Get(storage_.get(), i); }

 private:
  void Reallocate(uint16_t new_capacity) {
    auto new_storage = std::make_unique<Storage[]>(new_capacity);

    for (uint16_t i = 0u; i < size_; ++i) {
      new (Get(new_storage.get(), i)) T(std::move(*Get(storage_.get(), i)));
      Get(storage_.get(), i)->~T();
    }

    storage_ = std::move(new_storage);
    capacity_ = new_capacity;
  }

  static T* Get(Storage* storage, uint16_t i) {
    return std::launder(reinterpret_cast<T*>(&storage[i]));
  }

  std::unique_ptr<Storage[]> storage_;
  uint16_t size_;
  uint16_t capacity_;
};

static uint64_t constexpr N_TREES = 37u;
static uint64_t constexpr TREE_MASK = 0x1FFFFFFFFF;

static bool constexpr VALIDATE_MOVES = true;

inline u_int popcnt(uint64_t v) {
  return static_cast<u_int>(__builtin_popcountll(v));
}

#define __stringize(a) #a
#define stringize(a) __stringize(a)

#define ASSERT(cond)                                            \
  do {                                                          \
    if constexpr (VALIDATE_MOVES) {                             \
      if (!(cond)) {                                            \
        throw std::logic_error(stringize(__LINE__) ": " #cond); \
      }                                                         \
    }                                                           \
  } while (false)

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

class Move {
 public:
  enum class Type { kWait, kGrow, kComplete, kSeed };

  Type GetType() const {
    return static_cast<Type>((destination_ & 0b11000000) >> 6);
  }
  uint8_t GetDestination() const { return destination_ & 0b00111111; }
  uint8_t GetTarget() const { return target_; }

  static Move Wait() {
    Move m;
    m.SetType(Type::kWait);
    return m;
  }

  static Move Grow(uint8_t target) {
    Move m;
    m.SetType(Type::kGrow);
    m.SetTarget(target);
    return m;
  }

  static Move Complete(uint8_t target) {
    Move m;
    m.SetType(Type::kComplete);
    m.SetTarget(target);
    return m;
  }

  static Move Seed(uint8_t target, uint8_t destination) {
    Move m;
    m.SetType(Type::kSeed);
    m.SetTarget(target);
    m.SetDestination(destination);
    return m;
  }

  static Move Invalid() {
    Move m;
    m.target_ |= 0b10000000;
    return m;
  }

  bool IsValid() const { return (target_ & 0b10000000) ? false : true; }

  static uint16_t ToInt(Move const& m) {
    uint16_t i = 0u;
    i |= m.target_;
    i |= (static_cast<uint16_t>(m.destination_) << 8);
    return i;
  }

  static Move FromInt(uint16_t i) {
    Move m;
    m.target_ = static_cast<uint8_t>(i & 0xFF);
    m.destination_ = static_cast<uint8_t>(i >> 8);
    return m;
  }

 private:
  void SetTarget(uint8_t t) { target_ = t; }

  void SetDestination(uint8_t destination) {
    destination_ &= ~0b00111111;
    destination_ |= destination;
  }

  void SetType(Type t) {
    destination_ &= ~0b11000000;
    destination_ |= (static_cast<uint8_t>(t) << 6);
  }

  uint8_t target_{0u};
  uint8_t destination_{0u};
};

std::ostream& operator<<(std::ostream& os, Move const& move) {
  switch (move.GetType()) {
    case Move::Type::kWait:
      os << "WAIT";
      break;
    case Move::Type::kGrow:
      os << "GROW " << static_cast<uint16_t>(move.GetTarget());
      break;
    case Move::Type::kComplete:
      os << "COMPLETE " << static_cast<uint16_t>(move.GetTarget());
      break;
    case Move::Type::kSeed:
      os << "SEED " << static_cast<uint16_t>(move.GetTarget()) << " "
         << static_cast<uint16_t>(move.GetDestination());
      break;
    default:
      throw std::logic_error("Invalid Type!");
  }
  return os;
}

class GameState {
 public:
  static void ReadBoard(std::istream& input) {
    arid_ = 0u;
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
        arid_ |= GetTree(index);
      }
    }
  }

  static GameState FromStream(std::istream& input, TimeStamp& start) {
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
    }

    if constexpr (VALIDATE_MOVES) {
      MoveFilterParams params = MoveFilterParams::Default();
      params.block_self_neighbor = false;
      size_t expected_actions = 0;
      g.GetMoves(0, [&](Move const& m) { expected_actions++; }, params);
      ASSERT(n_actions == expected_actions);
    }

    return g;
  }

  static GameState RandomStart() {
    GameState g;

    arid_ = 0u;

    int n_desired_holes = std::rand() % 11;
    int n_holes = 0;
    std::vector<u_int> options;
    for (u_int i = 19; i < N_TREES; ++i) {
      options.push_back(i);
    }
    while (n_holes < n_desired_holes - 1) {
      u_int candidate = std::rand() % N_TREES;

      if (GetTree(candidate) & arid_) {
        /* Already arid. */
        continue;
      }

      arid_ |= GetTree(candidate);
      n_holes++;
      options.erase(std::remove(options.begin(), options.end(), candidate),
                    options.end());
      if (candidate > 0) {
        arid_ |= GetTree(OPPOSITE[candidate]);
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
      u_int first = new_options[std::rand() % new_options.size()];

      new_options.erase(
          std::remove(new_options.begin(), new_options.end(), first),
          new_options.end());
      new_options.erase(
          std::remove(new_options.begin(), new_options.end(), OPPOSITE[first]),
          new_options.end());

      u_int second = new_options[std::rand() % new_options.size()];
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
      u_int player,
      MoveFilterParams const params = MoveFilterParams::Default()) const {
    std::vector<Move> moves;
    GetMoves(player, [&](Move const& m) { moves.emplace_back(m); }, params);
    return moves;
  }

  template <class Callable>
  void GetMoves(
      u_int player, Callable&& callable,
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
                    SEED_DESTINATIONS[s - 1][target] & ~block & ~GetArid();
                IterateTrees(dest, [&](u_int destination) {
                  callable(Move::Seed(static_cast<uint8_t>(target),
                                      static_cast<uint8_t>(destination)));
                });
              });
        }
      }
    }
  }

  void Turn(u_int player, Move const& m) {
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
              ASSERT(GetTree(m.GetDestination()) & ~arid_);
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
        ASSERT(GetTree(m.GetDestination()) & ~arid_);
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
    return popcnt(GetPlayerTrees(player, size));
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

 private:
  void SetWaiting(u_int player) { owner_[player] |= (1ull << 63); }
  bool IsWaiting(u_int player) const { return owner_[player] & (1ull << 63); }
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

  uint64_t GetPlayerTrees(u_int player) const {
    return GetAllTrees() & owner_[player];
  }

  uint64_t GetTrees(uint size) const { return TREE_MASK & trees_[size]; }

  uint64_t GetPlayerTrees(u_int player, u_int size) const {
    return GetTrees(size) & owner_[player];
  }

  uint64_t GetAllTrees() const {
    return (trees_[0] | trees_[1] | trees_[2] | trees_[3]) & TREE_MASK;
  }

  uint64_t GetDormant() const { return dormant_; }
  static uint64_t GetArid() { return arid_; }

  template <class Callable>
  void IterateTrees(uint64_t trees, Callable&& callable) const {
    while (trees != 0) {
      int offset = __builtin_ctzll(trees);
      callable(static_cast<u_int>(offset));
      trees &= ~(1ull << offset);
    }
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
    PlaceTree(offset, player, 1, false);
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
          sun_points[player] += popcnt(valid_trees) * s;
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
  uint8_t GetNutrients() const { return GetByte(48u, trees_[0]); }
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
  static uint64_t arid_;
};

uint64_t GameState::arid_ = 0u;

class Agent {
 public:
  Agent(std::istream& input, std::ostream& output)
      : input_(input), output_(output) {}

  void Turn() {
    TimeStamp start;
    GameState state = GameState::FromStream(input_, start);

    Move move = ChooseMove(state, start);

    std::cerr << "D: " << start.Since() << std::endl;

    output_ << move << std::endl;
  }

  virtual Move ChooseMove(GameState const& state, TimeStamp const& start) = 0;

 private:
  std::istream& input_;
  std::ostream& output_;
};

class Mcts : public Agent {
 public:
  struct Node {
    GameState gs;
    std::vector<Node> children;
    std::vector<Move> unexplored;
    uint32_t n_rollouts{};
    float score{};
    Move preceeding;

    Node(GameState const& a_gs, Move const& a_preceeding = Move::Invalid())
        : gs(a_gs), unexplored(GetTreeMoves(gs)), preceeding(a_preceeding) {}
  };

  Mcts(std::istream& input, std::ostream& output) : Agent(input, output) {}

  Move ChooseMove(GameState const& state, TimeStamp const& start) override {
    if (!TrackActualAction(state)) {
      root_.emplace(state);
    }

    auto& root = root_.value();

    static bool first_turn = true;

    std::vector<Node*> expand_path;
    while (root.n_rollouts <= 0xFFFFFE) {
      expand_path.clear();
      expand_path.push_back(&root);
      Select(expand_path, true);

      if (!expand_path.back()->unexplored.empty()) {
        Expand(expand_path);
      }

      float score = Simulate(expand_path.back()->gs);
      Backup(expand_path, score);

      if (start.Since() >= (first_turn ? 0.995 : 0.095)) {
        break;
      }
    }
    first_turn = false;
    std::cerr << "R: " << root.n_rollouts << std::endl;
    ASSERT(!root.children.empty());

    auto best = MaxElement(
        root.children.begin(), root.children.end(), [](Node const& n) {
          std::cerr << (n.score / n.n_rollouts) << " " << n.n_rollouts << " "
                    << n.preceeding << std::endl;
          ASSERT(n.n_rollouts > 0);
          return n.score / n.n_rollouts;
        });

    Node local = std::move(*best);
    root_ = std::move(local);
    return root_.value().preceeding;
  }

 private:
  std::optional<Node> root_;
  Move last_{Move::Invalid()};

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
      std::cerr << "Op is waiting, reused " << root.n_rollouts << " trials"
                << std::endl;
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
    std::cerr << "Op did: " << new_root->preceeding << " reused "
              << new_root->n_rollouts << " trials" << std::endl;
    Node local = std::move(*new_root);
    root_ = std::move(local);
    return true;
  }

  static void Select(std::vector<Node*>& path, bool is_maximizing) {
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

    auto max = MaxElement(
        back.children.begin(), back.children.end(), [&](Node const& child) {
          ASSERT(child.n_rollouts > 0);

          float value =
              factor * child.score / child.n_rollouts +
              std::sqrt(2 * std::log(back.n_rollouts) / child.n_rollouts);
          return value;
        });

    /* Grow path and keep trying to find something to expand. */
    path.push_back(&*max);
    Select(path, !is_maximizing);
  }

  static void Expand(std::vector<Node*>& path) {
    auto& unexplored = path.back()->unexplored;
    auto it = unexplored.begin() + std::rand() % unexplored.size();
    auto move = *it;
    unexplored.erase(it);

    GameState next(path.back()->gs);
    next.Turn(next.NextPlayer(), move);
    path.back()->children.emplace_back(next, move);
    path.emplace_back(&path.back()->children.back());
  }

  static std::vector<Move> GetRawMoves(GameState const& g) {
    return g.GetMoves(g.NextPlayer());
  }

  static std::vector<Move> GetTreeMoves(GameState const& g) {
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

    g.GetMoves(g.NextPlayer(), filter, params);

    if ((!can_seed || day > 21) && !g.IsTerminal()) {
      moves.emplace_back(Move::Wait());
    }

    return moves;
  }

  static std::vector<Move> GetRolloutMoves(GameState const& g) {
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

    g.GetMoves(g.NextPlayer(), filter, params);

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

  static float Simulate(GameState const& state) {
    GameState g = state;
    while (!g.IsTerminal()) {
      auto moves = GetRolloutMoves(g);
      g.Turn(g.NextPlayer(), moves[std::rand() % moves.size()]);
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

int main() {
  GameState::ReadBoard(std::cin);
  Mcts agent(std::cin, std::cout);

  while (true) {
    agent.Turn();
  }

  return 0;
}

// int main() {
//   for (auto i = 0; i < 20000; ++i) {
//     auto g = GameState::RandomStart();

//     while (!g.IsTerminal()) {
//       auto moves = g.GetMoves(g.NextPlayer());
//       g.Turn(g.NextPlayer(), moves[std::rand() % moves.size()]);
//     }
//   }
// }