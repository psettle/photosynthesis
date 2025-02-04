#ifndef __INCLUDE_GUARD_NEURAL_NETWORK
#define __INCLUDE_GUARD_NEURAL_NETWORK

#include <fstream>
#include <memory>
#include <vector>

#include "neural_ILayer.hpp"
#include "neural_ILoss.hpp"
#include "neural_Linear.hpp"
#include "neural_Matrix.hpp"
#include "neural_ReLU.hpp"
#include "neural_Tanh.hpp"

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

  FloatTensor Forward(FloatTensor&& batch) const {
    FloatTensor current = std::move(batch);

    for (auto const& l : layers_) {
      current = const_cast<ILayer const*>(l.get())->Forward(std::move(current));
    }

    return current;
  }

  std::unique_ptr<Network> Clone() const {
    SaveToFile("temp.bin");
    return std::make_unique<Network>(LoadFromFile("temp.bin"));
  }

  float Loss(FloatTensor const& batch_input, FloatTensor const& truth) {
    auto result = Forward(batch_input);
    auto loss = loss_->Forward(result, truth);
    return loss;
  }

  float Batch(FloatTensor const& batch_input, FloatTensor const& truth,
              float learn_rate) {
    auto result = Forward(batch_input);
    auto loss = Loss(batch_input, truth);

    auto gradient = loss_->Backwards();
    for (auto it = layers_.rbegin(); it != layers_.rend(); ++it) {
      gradient = (*it)->Backwards(gradient, learn_rate);
    }

    return loss;
  }

  void SaveToFile(std::string const& fname) const {
    std::ofstream out;
    out.open(fname);

    char byte = static_cast<char>(layers_.size());
    out.write(&byte, 1);

    for (auto const& l : layers_) {
      l->Serialize(out);
    }

    out.close();
  }

  static Network LoadFromFile(std::string const& fname) {
    std::ifstream in;
    in.open(fname);

    char byte;
    in.read(&byte, 1);

    std::vector<std::unique_ptr<ILayer>> layers;
    while (byte-- > 0) {
      layers.emplace_back(ReadLayer(in));
    }

    in.close();

    return Network(std::move(layers), std::make_unique<MeanSquareError>());
  }

 private:
  static std::unique_ptr<ILayer> ReadLayer(std::istream& in) {
    char byte;
    in.read(&byte, 1);

    if (byte == 'L') {
      return Linear::Deserialize(in);
    } else if (byte == 'R') {
      return ReLU::Deserialize(in);
    } else if (byte == 'T') {
      return TanHActivation::Deserialize(in);
    } else {
      ASSERT(false);
      return nullptr;
    }
  }

  std::vector<std::unique_ptr<ILayer>> layers_;
  std::unique_ptr<ILoss> loss_;
};

}  // namespace neural

#endif