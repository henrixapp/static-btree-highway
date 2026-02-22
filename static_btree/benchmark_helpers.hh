#pragma once
#include <limits>
#include <random>
#include <vector>
template <class T>
typename std::enable_if<std::is_integral<T>::value, std::vector<T>>::type gen_data(
    size_t s, std::mt19937_64& rng) {
  std::vector<T> points(s);
  std::uniform_int_distribution<T> dist(std::numeric_limits<T>::min(),
                                        std::numeric_limits<T>::max());
  for (auto& p : points) {
    p = dist(rng);
  }
  return points;
}

template <class T>
typename std::enable_if<std::is_floating_point<T>::value, std::vector<T>>::type gen_data(
    size_t s, std::mt19937_64& rng) {
  std::vector<T> points(s);
  std::uniform_real_distribution<double> dist(std::numeric_limits<T>::min(),
                                              std::numeric_limits<T>::max());
  for (auto& p : points) {
    p = static_cast<T>(dist(rng));
  }
  return points;
}