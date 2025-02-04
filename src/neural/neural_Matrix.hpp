#ifndef __INCLUDE_GUARD_NEURAL_MATRIX
#define __INCLUDE_GUARD_NEURAL_MATRIX

#include <array>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "util_General.hpp"

namespace neural {

template <class T>
class Matrix {
 public:
  Matrix() : Matrix(0u, 0u) {}

  Matrix(u_int n_rows, u_int n_cols)
      : storage_(n_rows * n_cols), dimensions_{n_rows, n_cols} {}

  Matrix(std::array<u_int, 2u> const& shape) : Matrix(shape[0], shape[1]) {}

  T Get(u_int r, u_int c) const {
    ASSERT(r < shape()[0]);
    ASSERT(c < shape()[1]);

    return storage_[r * dimensions_[1] + c];
  }
  T& Get(u_int r, u_int c) {
    ASSERT(r < shape()[0]);
    ASSERT(c < shape()[1]);

    return storage_[r * dimensions_[1] + c];
  }

  T constexpr Get(u_int i) const { return storage_[i]; }
  T& Get(u_int i) { return storage_[i]; }
  void Set(u_int i, T v) { storage_[i] = v; }

  std::array<u_int, 2> const& shape() const { return dimensions_; }

  u_int constexpr size() const { return dimensions_[0] * dimensions_[1]; }

  T value() const {
    ASSERT(storage_.size() == 1);
    return storage_[0];
  }

  Matrix transpose() const {
    Matrix result(dimensions_[1], dimensions_[0]);

    for (u_int r = 0; r < dimensions_[0]; ++r) {
      for (u_int c = 0; c < dimensions_[1]; ++c) {
        result.Get(c, r) = Get(r, c);
      }
    }

    return result;
  }

  Matrix& operator-=(Matrix const& other);

  template <class GetRand>
  void Randomize(GetRand get_rand) {
    for (auto& e : storage_) {
      e = get_rand();
    }
  }

  void Serialize(std::ostream& out) const {
    u_int n_d = 2;
    out.write(reinterpret_cast<char const*>(&n_d), sizeof(n_d));

    for (auto const& d : dimensions_) {
      out.write(reinterpret_cast<char const*>(&d), sizeof(d));
    }
    for (auto const& t : storage_) {
      out.write(reinterpret_cast<char const*>(&t), sizeof(t));
    }
  }

  static Matrix<T> Deserialize(std::istream& in) {
    u_int n_d;
    in.read(reinterpret_cast<char*>(&n_d), sizeof(n_d));
    ASSERT(n_d == 2);

    u_int r, c;
    in.read(reinterpret_cast<char*>(&r), sizeof(r));
    in.read(reinterpret_cast<char*>(&c), sizeof(c));

    Matrix<T> result(r, c);

    for (auto& t : result.storage_) {
      in.read(reinterpret_cast<char*>(&t), sizeof(t));
    }

    return result;
  }

 private:
  std::vector<T> storage_;
  std::array<u_int, 2> dimensions_;
};

template <class T>
bool SizeEqual(std::array<T, 2> const& left, std::array<T, 2> const& right) {
  if (left.size() != right.size()) {
    return false;
  }
  return 0 == memcmp(left.data(), right.data(), left.size() * sizeof(T));
}

template <class T, class Operator, class O = T>
void ElementWiseOperation(Matrix<T> const& left, Matrix<T> const& right,
                          Matrix<O>& output, Operator op) {
  ASSERT(SizeEqual(left.shape(), right.shape()));
  ASSERT(SizeEqual(output.shape(), right.shape()));

  for (u_int i = 0u; i < left.size(); ++i) {
    output.Set(i, op(left.Get(i), right.Get(i)));
  }
}

template <class T, class Operator, class O = T>
void BroadcastByRow(Matrix<T> const& left, Matrix<T> const& right,
                    Matrix<O>& output, Operator op) {
  ASSERT(left.shape()[1] == right.shape()[1]);
  ASSERT(right.shape()[0] == 1);

  for (u_int r = 0u; r < left.shape()[0]; ++r) {
    for (u_int c = 0u; c < left.shape()[1]; ++c) {
      output.Get(r, c) = op(left.Get(r, c), right.Get(0, c));
    }
  }
}

template <class T, class Operator, class O = T>
void UnaryOperation(Matrix<T> const& input, Matrix<O>& output, Operator op) {
  ASSERT(SizeEqual(input.shape(), output.shape()));

  for (u_int i = 0u; i < input.size(); ++i) {
    output.Set(i, op(input.Get(i)));
  }
}

template <class T>
T Dot(Matrix<T> const& left, Matrix<T> const& right) {
  ASSERT(SizeEqual(left.shape(), right.shape()));
  ASSERT(left.shape()[1] == 1);

  return MatMul(left.transpose(), right).value();
}

template <class T>
Matrix<T> MatMul(Matrix<T> const& left, Matrix<T> const& right) {
  ASSERT(left.shape()[1] == right.shape()[0]);

  Matrix<T> result(left.shape()[0], right.shape()[1]);

  for (u_int r_c = 0u; r_c < right.shape()[1]; ++r_c) {
    for (u_int l_r = 0u; l_r < left.shape()[0]; ++l_r) {
      for (u_int l_c = 0u; l_c < left.shape()[1]; ++l_c) {
        result.Get(l_r, r_c) += left.Get(l_r, l_c) * right.Get(l_c, r_c);
      }
    }
  }

  return result;
}

template <class T>
Matrix<T> Square(Matrix<T> const& input) {
  Matrix<T> result(input.shape());
  UnaryOperation(input, result, [](T i) { return i * i; });
  return result;
}

template <class T>
Matrix<T> TanH(Matrix<T> const& input) {
  Matrix<T> result(input.shape());
  UnaryOperation(input, result, [](T i) { return std::tanh(i); });
  return result;
}

template <class T>
Matrix<T> Max(Matrix<T> const& left, T right) {
  Matrix<T> result(left.shape());
  neural::UnaryOperation(left, result, [&](T l) { return std::max(l, right); });
  return result;
}

template <class T>
Matrix<bool> Ge(Matrix<T> const& left, T right) {
  Matrix<bool> result(left.shape());
  neural::UnaryOperation(left, result, [&](T l) -> bool { return l >= right; });
  return result;
}

template <class T>
Matrix<T> Ternary(Matrix<bool> const& cond, T t, T f) {
  Matrix<T> result(cond.shape());
  neural::UnaryOperation(cond, result, [&](bool b) -> T { return b ? t : f; });
  return result;
}

template <class T>
Matrix<T>& Matrix<T>::operator-=(Matrix<T> const& other) {
  ElementWiseOperation(*this, other, *this, [](T l, T r) { return l - r; });
  return *this;
}

template <class T>
Matrix<T> Sum(Matrix<T> const& input) {
  Matrix<T> result(1, input.shape()[1]);

  for (u_int r = 0u; r < input.shape()[0]; ++r) {
    for (u_int c = 0u; c < input.shape()[1]; ++c) {
      result.Get(0, c) += input.Get(r, c);
    }
  }

  return result;
}

using FloatTensor = Matrix<float>;

}  // namespace neural

template <class T>
neural::Matrix<T> operator+(neural::Matrix<T> const& left,
                            neural::Matrix<T> const& right) {
  neural::Matrix<T> result(left.shape());

  if (left.shape() == right.shape()) {
    neural::ElementWiseOperation(left, right, result,
                                 [](T l, T r) { return l + r; });
  } else {
    neural::BroadcastByRow(left, right, result, [](T l, T r) { return l + r; });
  }

  return result;
}

template <class T>
neural::Matrix<T> operator-(neural::Matrix<T> const& left,
                            neural::Matrix<T> const& right) {
  neural::Matrix<T> result(left.shape());
  neural::ElementWiseOperation(left, right, result,
                               [](T l, T r) { return l - r; });
  return result;
}

template <class T>
bool operator==(neural::Matrix<T> const& left, neural::Matrix<T> const& right) {
  neural::Matrix<bool> result(left.shape());
  bool ret = true;
  neural::ElementWiseOperation(left, right, result, [&](T l, T r) {
    ret = ret && (l == r);
    return true;
  });
  return ret;
}

template <class T>
neural::Matrix<T> operator*(neural::Matrix<T> const& left,
                            neural::Matrix<T> const& right) {
  neural::Matrix<T> result(left.shape());
  neural::ElementWiseOperation(left, right, result,
                               [](T l, T r) { return l * r; });
  return result;
}

template <class T>
neural::Matrix<T> operator+(neural::Matrix<T> const& left, T right) {
  neural::Matrix<T> result(left.shape());
  neural::UnaryOperation(left, result, [&](T l) { return l + right; });
  return result;
}

template <class T>
neural::Matrix<T> operator-(neural::Matrix<T> const& left, T right) {
  neural::Matrix<T> result(left.shape());
  neural::UnaryOperation(left, result, [&](T l) { return l - right; });
  return result;
}

template <class T>
neural::Matrix<T> operator*(neural::Matrix<T> const& left, T right) {
  neural::Matrix<T> result(left.shape());
  neural::UnaryOperation(left, result, [&](T l) { return l * right; });
  return result;
}

template <class T>
neural::Matrix<T> operator/(neural::Matrix<T> const& left, T right) {
  neural::Matrix<T> result(left.shape());
  neural::UnaryOperation(left, result, [&](T l) { return l / right; });
  return result;
}

#endif