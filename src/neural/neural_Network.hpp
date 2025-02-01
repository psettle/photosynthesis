#ifndef __INCLUDE_GUARD_NEURAL_NETWORK
#define __INCLUDE_GUARD_NEURAL_NETWORK

#include <memory>
#include <vector>

#include "neural_ILayer.hpp"
#include "neural_ILoss.hpp"
#include "neural_Matrix.hpp"

namespace neural {

class Network {
 public:
  using FloatTensor = Matrix<float>;

  Network(std::vector<std::unique_ptr<ILayer>>&& layers,
          std::unique_ptr<ILoss> loss)
      : layers_(std::move(layers)), loss_(std::move(loss)) {}

  FloatTensor Forward(FloatTensor const& batch) {
    FloatTensor current = batch;

    for (auto const& l : layers_) {
      current = l->Forward(current);
    }

    return current;
  }

  float Batch(FloatTensor const& batch_input, FloatTensor const& truth,
              float learn_rate) {
    auto result = Forward(batch_input);
    auto loss = loss_->Forward(result, truth);
    auto gradient = loss_->Backwards();

    for (auto it = layers_.rbegin(); it != layers_.rend(); ++it) {
      gradient = (*it)->Backwards(gradient, learn_rate);
    }

    return loss;
  }

 private:
  std::vector<std::unique_ptr<ILayer>> layers_;
  std::unique_ptr<ILoss> loss_;
};

}  // namespace neural

#endif