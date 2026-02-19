#include <algorithm>
#include <cstdint>

#include "benchmark/benchmark.h"
#include "static_btree/static_btree.hh"

template <typename ValueType>
std::vector<ValueType> gen_data(size_t s, size_t scale) {
  std::vector<ValueType> points(s);
  for (auto& p : points) {
    p = rand() % (std::numeric_limits<ValueType>::max() / scale);
  }
  return points;
}
template <typename ValueType>
static void StdLowerBound(benchmark::State& state) {
  srand(42);
  auto points = gen_data<ValueType>(state.range(0), 40);
  std::sort(points.begin(), points.end());
  auto queries = gen_data<ValueType>(state.range(0) / 2, 4);
  for (auto _ : state) {
    ValueType mask = 0;
    for (auto p : queries) {
      auto q = std::lower_bound(points.begin(), points.end(), p);
      mask ^= (q - points.begin());
    }
    state.counters["mask"] = mask;
  }
}
template <template <typename> typename Tree, typename ValueType>
static void TreeLowerBound(benchmark::State& state) {
  srand(42);
  auto points = gen_data<ValueType>(state.range(0), 40);
  std::sort(points.begin(), points.end());
  auto queries = gen_data<ValueType>(state.range(0) / 2, 4);
  Tree<ValueType> tree(points);

  for (auto _ : state) {
    ValueType mask = 0;
    for (auto p : queries) {
      auto s = tree.lower_bound(p);
      mask ^= s;
    }
    state.counters["mask"] = mask;
  }
}
BENCHMARK(TreeLowerBound<henrixapp::static_btree::N_AVX3::ImplicitStaticBTree, uint32_t>)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(1e6);

BENCHMARK(StdLowerBound<uint32_t>)->Arg(100)->Arg(1000)->Arg(10000)->Arg(1e6);