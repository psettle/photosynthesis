#include <iostream>

#include "neural_Linear.hpp"
#include "neural_Matrix.hpp"
#include "neural_MeanSquareError.hpp"
#include "neural_Network.hpp"
#include "neural_ReLU.hpp"
#include "util_General.hpp"

int main() {
  neural::Matrix<float> x(4, 2);
  neural::Matrix<float> y(4, 1);

  x.Get(0, 0) = 0.0f;
  x.Get(0, 1) = 0.0f;
  x.Get(1, 0) = 0.0f;
  x.Get(1, 1) = 1.0f;
  x.Get(2, 0) = 1.0f;
  x.Get(2, 1) = 0.0f;
  x.Get(3, 0) = 1.0f;
  x.Get(3, 1) = 1.0f;

  y.Get(0, 0) = 0.0f;
  y.Get(1, 0) = 1.0f;
  y.Get(2, 0) = 1.0f;
  y.Get(3, 0) = 0.0f;

  std::vector<std::unique_ptr<neural::ILayer>> layers;
  layers.emplace_back(std::make_unique<neural::Linear>(2, 16));
  layers.emplace_back(std::make_unique<neural::ReLU>());
  layers.emplace_back(std::make_unique<neural::Linear>(16, 32));
  layers.emplace_back(std::make_unique<neural::ReLU>());
  layers.emplace_back(std::make_unique<neural::Linear>(32, 64));
  layers.emplace_back(std::make_unique<neural::ReLU>());
  layers.emplace_back(std::make_unique<neural::Linear>(64, 128));
  layers.emplace_back(std::make_unique<neural::ReLU>());
  layers.emplace_back(std::make_unique<neural::Linear>(128, 256));
  layers.emplace_back(std::make_unique<neural::ReLU>());
  layers.emplace_back(std::make_unique<neural::Linear>(256, 128));
  layers.emplace_back(std::make_unique<neural::ReLU>());
  layers.emplace_back(std::make_unique<neural::Linear>(128, 64));
  layers.emplace_back(std::make_unique<neural::ReLU>());
  layers.emplace_back(std::make_unique<neural::Linear>(64, 32));
  layers.emplace_back(std::make_unique<neural::ReLU>());
  layers.emplace_back(std::make_unique<neural::Linear>(32, 16));
  layers.emplace_back(std::make_unique<neural::ReLU>());
  layers.emplace_back(std::make_unique<neural::Linear>(16, 1));

  neural::Network n(std::move(layers),
                    std::make_unique<neural::MeanSquareError>());

  for (u_int e = 0u; e < 100; ++e) {
    n.Batch(x, y, 0.1f);
  }

  auto result = n.Forward(x);

  for (u_int i = 0u; i < result.size(); ++i) {
    std::cout << result.Get(i) << std::endl;
  }
}
