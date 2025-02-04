#ifndef __INCLUDE_GUARD_UTIL_GENERAL_HPP
#define __INCLUDE_GUARD_UTIL_GENERAL_HPP

#include <cfloat>
#include <chrono>
#include <cmath>

namespace util {

/**
 * Abstraction over platform specific popcnt (Number of 1 bits)
 */
inline u_int popcnt(uint64_t v) {
  return static_cast<u_int>(__builtin_popcountll(v));
}

static bool constexpr ENABLE_ASSERTS = false;

template <class It, class Evaluate>
It MaxElement(It begin, It end, Evaluate&& evaluate) {
  auto best = end;
  float best_score = -FLT_MAX;

  for (auto it = begin; it != end; ++it) {
    float score = evaluate(*it);
    if (score > best_score) {
      best = it;
      best_score = score;
    }
  }

  return best;
}

}  // namespace util

#define __stringize(a) #a
#define stringize(a) __stringize(a)

#define ASSERT(cond)                                                     \
  do {                                                                   \
    if constexpr (::util::ENABLE_ASSERTS) {                              \
      if (!(cond)) {                                                     \
        throw std::logic_error(__FILE__ stringize(__LINE__) ": " #cond); \
      }                                                                  \
    }                                                                    \
  } while (false)

#endif /* __INCLUDE_GUARD_UTIL_GENERAL_HPP */
