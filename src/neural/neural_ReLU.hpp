#ifndef __INCLUDE_GUARD_NEURAL_RELU
#define __INCLUDE_GUARD_NEURAL_RELU

#include "neural_ILayer.hpp"
#include "neural_Matrix.hpp"

namespace neural {

class ReLU : public ILayer {
 public:
  FloatTensor Forward(FloatTensor const& input) override {
    input_ = input;
    return Max(input_, 0.0f);
  }

  FloatTensor Forward(FloatTensor&& input) const override {
    return Max(std::move(input), 0.0f);
  }

  FloatTensor Backwards(FloatTensor const& output_gradient,
                        float learn_rate) override {
    auto ge = Ge(input_, 0.0f);
    auto input_gradient = output_gradient * Ternary(ge, 1.0f, 0.0f);
    return input_gradient;
  }

  void Serialize(std::ostream& out) const override {
    char byte = 'R';
    out.write(&byte, 1);
  }
  static std::unique_ptr<ReLU> Deserialize(std::istream& in) {
    return std::make_unique<ReLU>();
  }

 private:
  FloatTensor input_;
};

}  // namespace neural

#endif