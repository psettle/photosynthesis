#ifndef __INCLUDE_GUARD_NEURAL_TANH
#define __INCLUDE_GUARD_NEURAL_TANH

#include "neural_ILayer.hpp"
#include "neural_Matrix.hpp"

namespace neural {

class TanHActivation : public ILayer {
 public:
  FloatTensor Forward(FloatTensor const& input) override {
    input_ = TanH(input);
    return input_;
  }

  FloatTensor Forward(FloatTensor&& input) const override {
    return TanH(input);
  }

  FloatTensor Backwards(FloatTensor const& output_gradient,
                        float learn_rate) override {
    return (Square(input_) - 1.0f) * -1.0f;
  }

  void Serialize(std::ostream& out) const override {
    char byte = 'T';
    out.write(&byte, 1);
  }
  static std::unique_ptr<TanHActivation> Deserialize(std::istream& in) {
    return std::make_unique<TanHActivation>();
  }

 private:
  FloatTensor input_;
};

}  // namespace neural

#endif