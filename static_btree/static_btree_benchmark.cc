#include <algorithm>
#include <cstdint>
#include <vector>

#include "benchmark/benchmark.h"

#undef HWY_TARGET_INCLUDE
// For dynamic dispatch, specify the name of the current file (unfortunately
// __FILE__ is not reliable) so that foreach_target.h can re-include it.
#define HWY_TARGET_INCLUDE "static_btree/static_btree_benchmark.cc"
// Generates code for each enabled target by re-including this source file.

#include "hwy/foreach_target.h"  // IWYU pragma: keep
#include "hwy/highway.h"
#include "static_btree/static_btree.hh"

HWY_BEFORE_NAMESPACE();
namespace bench {
namespace HWY_NAMESPACE {
template <typename ValueType>
std::vector<ValueType> gen_data(size_t s, size_t scale) {
  std::vector<ValueType> points(s);
  for (auto& p : points) {
    p = rand() % (std::numeric_limits<ValueType>::max() / scale);
  }
  return points;
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

template <typename DataType>
void RunStaticBTreeInternal(benchmark::State& st) {
  TreeLowerBound<henrixapp::static_btree::HWY_NAMESPACE::ImplicitStaticBTree, DataType>(st);
}
}  // namespace HWY_NAMESPACE
}  // namespace bench
HWY_AFTER_NAMESPACE();
#if HWY_ONCE
namespace bench {
template <typename DataType>
static void StaticBTreeBenchmark(benchmark::State& st) {
  HWY_EXPORT_AND_DYNAMIC_DISPATCH_T(RunStaticBTreeInternal<DataType>)(st);
}
BENCHMARK_TEMPLATE(StaticBTreeBenchmark, uint32_t)->Arg(100)->Arg(1000)->Arg(10000)->Arg(1e6);
BENCHMARK_TEMPLATE(StaticBTreeBenchmark, uint64_t)->Arg(100)->Arg(1000)->Arg(10000)->Arg(1e6);
BENCHMARK_TEMPLATE(StaticBTreeBenchmark, uint16_t)->Arg(100)->Arg(1000)->Arg(10000)->Arg(1e6);

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
BENCHMARK(StdLowerBound<uint32_t>)->Arg(100)->Arg(1000)->Arg(10000)->Arg(1e6);
BENCHMARK(StdLowerBound<uint64_t>)->Arg(100)->Arg(1000)->Arg(10000)->Arg(1e6);
BENCHMARK(StdLowerBound<uint16_t>)->Arg(100)->Arg(1000)->Arg(10000)->Arg(1e6);

}  // namespace bench

#endif