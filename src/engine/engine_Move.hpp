#ifndef __INCLUDE_GUARD_ENGINE_MOVE_HPP
#define __INCLUDE_GUARD_ENGINE_MOVE_HPP

#include <cstdint>
#include <iostream>

namespace engine {

class Move {
 public:
  enum class Type { kWait, kGrow, kComplete, kSeed };

  Type GetType() const {
    return static_cast<Type>((destination_ & 0b11000000) >> 6);
  }
  uint8_t GetDestination() const { return destination_ & 0b00111111; }
  uint8_t GetTarget() const { return target_; }

  static Move Wait() {
    Move m;
    m.SetType(Type::kWait);
    return m;
  }

  static Move Grow(uint8_t target) {
    Move m;
    m.SetType(Type::kGrow);
    m.SetTarget(target);
    return m;
  }

  static Move Complete(uint8_t target) {
    Move m;
    m.SetType(Type::kComplete);
    m.SetTarget(target);
    return m;
  }

  static Move Seed(uint8_t target, uint8_t destination) {
    Move m;
    m.SetType(Type::kSeed);
    m.SetTarget(target);
    m.SetDestination(destination);
    return m;
  }

  static Move Invalid() {
    Move m;
    m.target_ |= 0b10000000;
    return m;
  }

  bool IsValid() const { return (target_ & 0b10000000) ? false : true; }

  static uint16_t ToInt(Move const& m) {
    uint16_t i = 0u;
    i |= m.target_;
    i |= (static_cast<uint16_t>(m.destination_) << 8);
    return i;
  }

  static Move FromInt(uint16_t i) {
    Move m;
    m.target_ = static_cast<uint8_t>(i & 0xFF);
    m.destination_ = static_cast<uint8_t>(i >> 8);
    return m;
  }

 private:
  void SetTarget(uint8_t t) { target_ = t; }

  void SetDestination(uint8_t destination) {
    destination_ &= ~0b00111111;
    destination_ |= destination;
  }

  void SetType(Type t) {
    destination_ &= ~0b11000000;
    destination_ |= (static_cast<uint8_t>(t) << 6);
  }

  uint8_t target_{0u};
  uint8_t destination_{0u};
};

}  // namespace engine

std::ostream& operator<<(std::ostream& os, engine::Move const& move) {
  switch (move.GetType()) {
    case engine::Move::Type::kWait:
      os << "WAIT";
      break;
    case engine::Move::Type::kGrow:
      os << "GROW " << static_cast<uint16_t>(move.GetTarget());
      break;
    case engine::Move::Type::kComplete:
      os << "COMPLETE " << static_cast<uint16_t>(move.GetTarget());
      break;
    case engine::Move::Type::kSeed:
      os << "SEED " << static_cast<uint16_t>(move.GetTarget()) << " "
         << static_cast<uint16_t>(move.GetDestination());
      break;
    default:
      throw std::logic_error("Invalid Type!");
  }
  return os;
}

#endif /* __INCLUDE_GUARD_ENGINE_MOVE_HPP */