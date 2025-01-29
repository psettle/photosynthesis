#ifndef __INCLUDE_GUARD_TEST_RUNNER_HPP
#define __INCLUDE_GUARD_TEST_RUNNER_HPP

#include <iostream>
#include <memory>

#include "engine_Agent.hpp"
#include "engine_Referee.hpp"
#include "pthread.h"

namespace test {

namespace internal {
struct TestControl {
  std::unique_ptr<engine::Agent> left;
  std::unique_ptr<engine::Agent> right;
  u_int n_games;
  u_int left_wins{};
  u_int right_wins{};
  pthread_t thread{};
  u_int index;
};

static void* RunTestsThread(void* ptr) {
  TestControl& ctrl = *static_cast<TestControl*>(ptr);

  bool is_prime = false;
  if (ctrl.index == 0) {
    is_prime = true;
  }

  for (size_t i = 0u; i < ctrl.n_games; ++i) {
    auto result = engine::Referee::PlayGame(*ctrl.left, *ctrl.right);
    if (result.winner == 0) {
      ctrl.left_wins++;
    } else if (result.winner == 1) {
      ctrl.right_wins++;
    }

    if (is_prime) {
      std::cerr << (i + 1) << "/" << ctrl.n_games << std::endl;
    }
  }

  return NULL;
}
}  // namespace internal

struct TestResult {
  u_int left_wins{0u};
  u_int right_wins{0u};
  u_int n_games{0u};
};

template <class Left, class Right>
TestResult Test(size_t n_samples, size_t n_threads) {
  std::vector<internal::TestControl> tests(n_threads);

  for (u_int i = 0; i < n_threads; ++i) {
    auto& test = tests[i];
    test.left = std::make_unique<Left>();
    test.right = std::make_unique<Right>();
    test.n_games = n_samples / n_threads;
    test.index = i;
    pthread_create(&test.thread, NULL, internal::RunTestsThread, &test);
  }

  TestResult result{};

  for (auto& test : tests) {
    pthread_join(test.thread, NULL);
    result.left_wins += test.left_wins;
    result.right_wins += test.right_wins;
    result.n_games += test.n_games;
  }

  return result;
}

}  // namespace test

#endif /* __INCLUDE_GUARD_TEST_RUNNER_HPP */