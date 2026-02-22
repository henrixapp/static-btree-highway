#include <algorithm>
#include <cstdint>
#include <limits>
#include <vector>

#include "benchmark/benchmark.h"
#include "static_btree/benchmark_helpers.hh"

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
template <typename DataType>
void RunStaticBTreeInternal(benchmark::State& st) {
  TreeLowerBound<henrixapp::static_btree::HWY_NAMESPACE::ImplicitStaticBTree, DataType>(st);
}
}  // namespace HWY_NAMESPACE
}  // namespace bench
HWY_AFTER_NAMESPACE();
#if HWY_ONCE
namespace bench {
template <typename DT>
void BestImplementation(benchmark::State& st) {
  HWY_EXPORT_AND_DYNAMIC_DISPATCH_T(RunStaticBTreeInternal<DT>)(st);
}
BENCHMARK(bench::BestImplementation<int>)->Arg(100);
template <template <typename> typename Tree, typename ValueType>
static void TreeLowerBound(benchmark::State& state) {
  srand(42);
  std::mt19937_64 rng(rand());
  auto points = gen_data<ValueType>(state.range(0), rng);
  std::sort(points.begin(), points.end());
  auto queries = gen_data<ValueType>(1e6, rng);
  Tree<ValueType> tree(points);
  size_t mask = 0;
  for (auto _ : state) {
    mask = 0;
    for (auto p : queries) {
      auto s = tree.lower_bound(p);
      mask ^= s;
      benchmark::DoNotOptimize(mask);
    }
  }
  state.counters["mask"] = mask;
}
#define MY_BENCH_MACRO(T, N, TYPE)                                                     \
  BENCHMARK(TreeLowerBound<henrixapp::static_btree::N::ImplicitStaticBTree, TYPE>)     \
      ->Name("StaticBTree," #N "," #TYPE "," +                                         \
             std::to_string(henrixapp::static_btree::N::ImplicitStaticBTree<TYPE>::B)) \
      ->Arg(100)                                                                       \
      ->Arg(1000)                                                                      \
      ->Arg(10000)                                                                     \
      ->Arg(1e6)                                                                       \
      ->Arg(1e7);

#define RUN_ALL_BENCHMARKS(T, N) \
  MY_BENCH_MACRO(T, N, uint8_t)  \
  MY_BENCH_MACRO(T, N, uint16_t) \
  MY_BENCH_MACRO(T, N, uint32_t) \
  MY_BENCH_MACRO(T, N, uint64_t) \
  MY_BENCH_MACRO(T, N, int8_t)   \
  MY_BENCH_MACRO(T, N, int16_t)  \
  MY_BENCH_MACRO(T, N, int32_t)  \
  MY_BENCH_MACRO(T, N, int64_t)

HWY_VISIT_TARGETS(RUN_ALL_BENCHMARKS);

template <typename ValueType>
static void StdLowerBound(benchmark::State& state) {
  srand(42);
  std::mt19937_64 rng(rand());
  auto points = gen_data<ValueType>(state.range(0), rng);
  std::sort(points.begin(), points.end());
  auto queries = gen_data<ValueType>(1e6, rng);
  ValueType mask;
  for (auto _ : state) {
    mask = 0;
    for (auto p : queries) {
      auto q = std::lower_bound(points.begin(), points.end(), p);
      mask ^= (q - points.begin());
      benchmark::DoNotOptimize(mask);
    }
  }
  state.counters["mask"] = mask;
}
#define STD_BENCHMARK(DT)          \
  BENCHMARK(StdLowerBound<DT>)     \
      ->Name("StdLowerbound," #DT) \
      ->Arg(100)                   \
      ->Arg(1000)                  \
      ->Arg(10000)                 \
      ->Arg(1e6)                   \
      ->Arg(1e7);
STD_BENCHMARK(uint8_t);
STD_BENCHMARK(uint16_t);
STD_BENCHMARK(uint32_t);
STD_BENCHMARK(uint64_t);
STD_BENCHMARK(int8_t);
STD_BENCHMARK(int16_t);
STD_BENCHMARK(int32_t);
STD_BENCHMARK(int64_t);
}  // namespace bench

#endif