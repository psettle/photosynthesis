#ifndef __INCLUDE_GUARD_NEURAL_LOSS
#define __INCLUDE_GUARD_NEURAL_LOSS

#include "neural_Matrix.hpp"

namespace neural {

class ILoss {
 public:
  virtual ~ILoss() = default;

  virtual float Forward(FloatTensor const& actual,
                        FloatTensor const& expected) = 0;

  virtual FloatTensor Backwards() = 0;
};

}  // namespace neural

#endif