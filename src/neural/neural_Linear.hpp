#ifndef __INCLUDE_GUARD_NEURAL_LINEAR
#define __INCLUDE_GUARD_NEURAL_LINEAR

#include <memory>
#include <random>

#include "neural_ILayer.hpp"
#include "neural_Matrix.hpp"

namespace neural {

class Linear : public ILayer {
 public:
  Linear(u_int input_dim, u_int output_dim)
      : w_(input_dim, output_dim), b_(1, output_dim) {
    /* Glorot Initialization */
    float x = std::sqrt(6.0 / (input_dim + output_dim));
    std::uniform_real_distribution<float> uniform_dist(-x, x);

    std::default_random_engine rand_engine(std::random_device{}());
    auto rand_func = [&]() -> float { return uniform_dist(rand_engine); };

    w_.Randomize(rand_func);
    b_.Randomize(rand_func);
  }

  Linear(FloatTensor&& w, FloatTensor&& b) : w_(w), b_(b) {}

  FloatTensor Forward(FloatTensor const& input) override {
    input_ = input;
    return MatMul(input, w_) + b_;
  }

  FloatTensor Forward(FloatTensor&& input) const override {
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

  void Serialize(std::ostream& out) const override {
    char byte = 'L';
    out.write(&byte, 1);
    w_.Serialize(out);
    b_.Serialize(out);
  }

  static std::unique_ptr<Linear> Deserialize(std::istream& in) {
    auto w = FloatTensor::Deserialize(in);
    auto b = FloatTensor::Deserialize(in);
    return std::make_unique<Linear>(std::move(w), std::move(b));
  }

 private:
  FloatTensor w_;
  FloatTensor b_;
  FloatTensor input_;
};

}  // namespace neural

#endif