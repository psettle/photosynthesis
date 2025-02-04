#ifndef __INCLUDE_GUARD_AGENT_DATAPOINT_HPP
#define __INCLUDE_GUARD_AGENT_DATAPOINT_HPP

#include <fstream>
#include <iostream>
#include <string>

#include "engine_GameState.hpp"

namespace agent {
struct DataPoint {
  engine::GameState state;
  uint64_t arid;
  uint8_t p0_score;
  uint8_t p1_score;
  uint8_t winner;

  void Serialize(std::ostream& out) const {
    state.Serialize(out);
    out.write(reinterpret_cast<char const*>(&arid), sizeof(arid));
    out.write(reinterpret_cast<char const*>(&p0_score), 1);
    out.write(reinterpret_cast<char const*>(&p1_score), 1);
    out.write(reinterpret_cast<char const*>(&winner), 1);
  }

  static DataPoint Deserialize(std::istream& in) {
    DataPoint dp{engine::GameState::Deserialize(in)};
    in.read(reinterpret_cast<char*>(&dp.arid), sizeof(arid));
    in.read(reinterpret_cast<char*>(&dp.p0_score), 1);
    in.read(reinterpret_cast<char*>(&dp.p1_score), 1);
    in.read(reinterpret_cast<char*>(&dp.winner), 1);
    return dp;
  }
};

std::vector<DataPoint> LoadDataPoints(std::string const& fname) {
  std::ifstream input;
  input.open(fname);

  std::vector<DataPoint> data;

  while (!input.eof()) {
    data.emplace_back(DataPoint::Deserialize(input));
  }

  input.close();

  return data;
}

}  // namespace agent

#endif /* __INCLUDE_GUARD_AGENT_DATAPOINT_HPP */