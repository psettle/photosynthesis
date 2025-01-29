#ifndef __INCLUDE_GUARD_UTIL_TIMESTAMP_HPP
#define __INCLUDE_GUARD_UTIL_TIMESTAMP_HPP

#include <chrono>

namespace util {
class TimeStamp {
 public:
  using Time = std::chrono::high_resolution_clock;

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
}  // namespace util

#endif /* __INCLUDE_GUARD_UTIL_TIMESTAMP_HPP */
