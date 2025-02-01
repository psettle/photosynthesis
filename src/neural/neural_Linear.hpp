#ifndef __INCLUDE_GUARD_NEURAL_LINEAR
#define __INCLUDE_GUARD_NEURAL_LINEAR

#include <random>

#include "neural_ILayer.hpp"
#include "neural_Matrix.hpp"

namespace neural {

class Linear : public ILayer {
 public:
  Linear(u_int input_dim, u_int output_dim)
      : w_(input_dim, output_dim), b_(1, output_dim) {
    std::random_device rand_device;
    std::default_random_engine rand_engine(rand_device());
    float x = std::sqrt(6.0 / (input_dim + output_dim));
    std::uniform_real_distribution<float> uniform_dist(-x, x);
    auto rand_func = [&]() -> float { return uniform_dist(rand_engine); };

    w_.Randomize(rand_func);
    b_.Randomize(rand_func);
  }

  FloatTensor Forward(FloatTensor const& input) override {
    input_ = input;
    return MatMul(input, w_) + b_;
  }

  FloatTensor Backwards(FloatTensor const& output_gradient,
                        float learn_rate) override {
    auto input_gradient = MatMul(output_gradient, w_.transpose());
    auto weight_gradient = MatMul(input_.transpose(), output_gradient);

    w_ -= weight_gradient * learn_rate;
    b_ -= Sum(output_gradient * learn_rate);

    return input_gradient;
  }

 private:
  FloatTensor w_;
  FloatTensor b_;
  FloatTensor input_;
};

}  // namespace neural

#endif