#pragma once
#include <limits>
#include <random>
template <typename ValueType>
std::vector<ValueType> gen_data(size_t s, std::mt19937_64& rng) {
  std::vector<ValueType> points(s);
  std::uniform_int_distribution<ValueType> dist(std::numeric_limits<ValueType>::min(),
                                                std::numeric_limits<ValueType>::max());
  for (auto& p : points) {
    p = dist(rng);
  }
  return points;
}
