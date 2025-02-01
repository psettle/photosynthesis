#ifndef __INCLUDE_GUARD_NEURAL_LAYER
#define __INCLUDE_GUARD_NEURAL_LAYER

#include "neural_Matrix.hpp"

namespace neural {

class ILayer {
 public:
  virtual ~ILayer() = default;

  virtual FloatTensor Forward(FloatTensor const& input) = 0;

  virtual FloatTensor Backwards(FloatTensor const& output_gradient,
                                float learn_rate) = 0;
};

}  // namespace neural

#endif