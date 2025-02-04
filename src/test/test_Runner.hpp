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
  engine::IAgentFactory const* left;
  engine::IAgentFactory const* right;
  u_int n_games;
  pthread_t thread{};
  u_int index;
  pthread_mutex_t* mutex;
  std::vector<engine::Episode>* all_episodes;
};

static void* RunTestsThread(void* ptr) {
  TestControl& ctrl = *static_cast<TestControl*>(ptr);

  bool is_prime = false;
  if (ctrl.index == 0) {
    is_prime = true;
  }

  for (size_t i = 0u; i < ctrl.n_games; ++i) {
    auto result = engine::Referee::CollectEpisode(*ctrl.left, *ctrl.right);

    pthread_mutex_lock(ctrl.mutex);
    ctrl.all_episodes->emplace_back(std::move(result));
    pthread_mutex_unlock(ctrl.mutex);

    if (is_prime) {
      // std::cerr << (i + 1) << "/" << ctrl.n_games << std::endl;
    }
  }

  return NULL;
}
}  // namespace internal

std::vector<engine::Episode> Test(engine::IAgentFactory const& left,
                                  engine::IAgentFactory const& right,
                                  size_t n_samples, size_t n_threads) {
  pthread_mutex_t mutex;
  pthread_mutex_init(&mutex, nullptr);

  std::vector<engine::Episode> episodes;

  std::vector<internal::TestControl> tests(n_threads);

  for (u_int i = 0; i < n_threads; ++i) {
    auto& test = tests[i];
    test.left = &left;
    test.right = &right;
    test.n_games = n_samples / n_threads;
    test.index = i;
    test.mutex = &mutex;
    test.all_episodes = &episodes;
    pthread_create(&test.thread, NULL, internal::RunTestsThread, &test);
  }

  for (auto& test : tests) {
    pthread_join(test.thread, NULL);
  }

  pthread_mutex_destroy(&mutex);

  return episodes;
}  // namespace test

}  // namespace test

#endif /* __INCLUDE_GUARD_TEST_RUNNER_HPP */