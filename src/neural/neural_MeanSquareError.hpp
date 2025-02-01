#ifndef __INCLUDE_GUARD_NEURAL_MEAN_SQUARE_ERROR
#define __INCLUDE_GUARD_NEURAL_MEAN_SQUARE_ERROR

#include "neural_ILoss.hpp"
#include "neural_Matrix.hpp"

namespace neural {

class MeanSquareError : public ILoss {
 public:
  float Forward(FloatTensor const& actual,
                FloatTensor const& expected) override {
    delta_ = expected - actual;

    return Sum(Square(delta_)).value() / actual.shape()[0];
  }

  FloatTensor Backwards() override {
    return delta_ * (-2.0f / delta_.shape()[0]);
  }

 private:
  FloatTensor delta_{};
};

}  // namespace neural

#endif