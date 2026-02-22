
#include <sys/types.h>

#include <cstddef>
#include <cstdint>

#include "static_btree/benchmark_helpers.hh"
#undef HWY_TARGET_INCLUDE
// For dynamic dispatch, specify the name of the current file (unfortunately
// __FILE__ is not reliable) so that foreach_target.h can re-include it.
#define HWY_TARGET_INCLUDE "static_btree/nano_bench.cc"
// Generates code for each enabled target by re-including this source file.

#include "hwy/foreach_target.h"  // IWYU pragma: keep
#include "hwy/highway.h"
#include "hwy/nanobenchmark.h"
#include "static_btree/benchmark_helpers.hh"
#include "static_btree/static_btree.hh"

#if HWY_ONCE
namespace nanobench {
template <class LowerBoundable>
struct Benchmark {
  using DataType = typename LowerBoundable::DataType;
  LowerBoundable instance;
  std::vector<std::vector<DataType>> queries;

  Benchmark(const std::vector<DataType>& inputs, const std::vector<std::vector<DataType>>& queries)
      : instance(inputs), queries(queries) {}
  size_t operator()(size_t i) {
    size_t Mask = 0;
    for (auto p : queries[i]) {
      Mask ^= instance.lower_bound(p);
    }
    return Mask;
  }
};
template <typename DS, int n_inputs, int n_queries, int num_per_query>
HWY_NOINLINE void RunBench(const std::string& name) {
  using DT = typename DS::DataType;
  srand(42);
  std::mt19937_64 rng(rand());
  hwy::Result results[n_queries];
  hwy::FuncInput input[n_queries];
  std::iota(input, input + n_queries, 0);
  std::vector<DT> points = gen_data<DT>(n_inputs, rng);
  std::vector<std::vector<DT>> queries(n_queries);
  for (auto& q : queries) {
    q = gen_data<DT>(num_per_query, rng);
  }
  std::sort(points.begin(), points.end());
  Benchmark<DS> benchmark(points, queries);
  hwy::Params params;
  params.verbose = false;
  params.max_evals = 7;
  params.target_rel_mad = 0.002;

  auto result_count = 0;

  do {
    result_count = hwy::MeasureClosure([&](const hwy::FuncInput val) { return benchmark(val); },
                                       input, n_queries, results, params);
  } while (result_count != n_queries);
  double ticks = 0;
  double var = 0;
  for (size_t i = 0; i < n_queries; i++) {
    ticks += results[i].ticks;
    var += results[i].variability;
  }
  std::cout << name << "," << ticks / n_queries << "," << var / n_queries << std::endl;
}
}  // namespace nanobench
template <typename DT>
struct StdLowerbounder {
  using DataType = DT;
  const std::vector<DataType> data;
  StdLowerbounder(const std::vector<DataType>& _data) : data(_data) {}
  size_t lower_bound(DT val) {
    return std::lower_bound(data.begin(), data.end(), val) - data.begin();
  }
};
int main() {
  static constexpr const int query_sets = 1;
  static constexpr const int queries_per_sets = 10000;

#define STD_LOWER_BOUND_BENCH(TYPE, ELEMS)                                            \
  nanobench::RunBench<StdLowerbounder<int32_t>, ELEMS, query_sets, queries_per_sets>( \
      #ELEMS ",StdLowerbound,STL," #TYPE                                              \
             ","                                                                      \
             "0");
  STD_LOWER_BOUND_BENCH(uint8_t, 100)
  STD_LOWER_BOUND_BENCH(uint8_t, 200)
  STD_LOWER_BOUND_BENCH(uint16_t, 100)
  STD_LOWER_BOUND_BENCH(uint16_t, 1000)
  STD_LOWER_BOUND_BENCH(uint16_t, 10000)
  STD_LOWER_BOUND_BENCH(uint16_t, 50000)
  STD_LOWER_BOUND_BENCH(uint32_t, 100)
  STD_LOWER_BOUND_BENCH(uint32_t, 1000)
  STD_LOWER_BOUND_BENCH(uint32_t, 10000)
  STD_LOWER_BOUND_BENCH(uint32_t, 100000)
  STD_LOWER_BOUND_BENCH(uint32_t, 1000000)
  STD_LOWER_BOUND_BENCH(uint32_t, 10000000)
  STD_LOWER_BOUND_BENCH(uint64_t, 100)
  STD_LOWER_BOUND_BENCH(uint64_t, 1000)
  STD_LOWER_BOUND_BENCH(uint64_t, 10000)
  STD_LOWER_BOUND_BENCH(uint64_t, 100000)
  STD_LOWER_BOUND_BENCH(uint64_t, 1000000)
  STD_LOWER_BOUND_BENCH(uint64_t, 10000000)
  STD_LOWER_BOUND_BENCH(int8_t, 100)
  STD_LOWER_BOUND_BENCH(int8_t, 200)
  STD_LOWER_BOUND_BENCH(int16_t, 100)
  STD_LOWER_BOUND_BENCH(int16_t, 1000)
  STD_LOWER_BOUND_BENCH(int16_t, 10000)
  STD_LOWER_BOUND_BENCH(int16_t, 50000)
  STD_LOWER_BOUND_BENCH(int32_t, 100)
  STD_LOWER_BOUND_BENCH(int32_t, 1000)
  STD_LOWER_BOUND_BENCH(int32_t, 10000)
  STD_LOWER_BOUND_BENCH(int32_t, 100000)
  STD_LOWER_BOUND_BENCH(int32_t, 1000000)
  STD_LOWER_BOUND_BENCH(int32_t, 10000000)
  STD_LOWER_BOUND_BENCH(int64_t, 100)
  STD_LOWER_BOUND_BENCH(int64_t, 1000)
  STD_LOWER_BOUND_BENCH(int64_t, 10000)
  STD_LOWER_BOUND_BENCH(int64_t, 100000)
  STD_LOWER_BOUND_BENCH(int64_t, 1000000)
  STD_LOWER_BOUND_BENCH(int64_t, 10000000)
#define BTREE_LOWER_BOUND_MACRO(T, N, TYPE, ELEMS)                                              \
  nanobench::RunBench<henrixapp::static_btree::N::ImplicitStaticBTree<TYPE>, ELEMS, query_sets, \
                      queries_per_sets>(                                                        \
      #ELEMS ",StaticBTree," #N "," #TYPE "," +                                                 \
      std::to_string(henrixapp::static_btree::N::ImplicitStaticBTree<TYPE>::B));

#define RUN_ALL_BENCHMARKS(T, N)                    \
  BTREE_LOWER_BOUND_MACRO(T, N, uint8_t, 100)       \
  BTREE_LOWER_BOUND_MACRO(T, N, uint8_t, 200)       \
  BTREE_LOWER_BOUND_MACRO(T, N, uint16_t, 100)      \
  BTREE_LOWER_BOUND_MACRO(T, N, uint16_t, 1000)     \
  BTREE_LOWER_BOUND_MACRO(T, N, uint16_t, 10000)    \
  BTREE_LOWER_BOUND_MACRO(T, N, uint16_t, 50000)    \
  BTREE_LOWER_BOUND_MACRO(T, N, uint32_t, 100)      \
  BTREE_LOWER_BOUND_MACRO(T, N, uint32_t, 1000)     \
  BTREE_LOWER_BOUND_MACRO(T, N, uint32_t, 10000)    \
  BTREE_LOWER_BOUND_MACRO(T, N, uint32_t, 100000)   \
  BTREE_LOWER_BOUND_MACRO(T, N, uint32_t, 1000000)  \
  BTREE_LOWER_BOUND_MACRO(T, N, uint32_t, 10000000) \
  BTREE_LOWER_BOUND_MACRO(T, N, uint64_t, 100)      \
  BTREE_LOWER_BOUND_MACRO(T, N, uint64_t, 1000)     \
  BTREE_LOWER_BOUND_MACRO(T, N, uint64_t, 10000)    \
  BTREE_LOWER_BOUND_MACRO(T, N, uint64_t, 100000)   \
  BTREE_LOWER_BOUND_MACRO(T, N, uint64_t, 1000000)  \
  BTREE_LOWER_BOUND_MACRO(T, N, uint64_t, 10000000) \
  BTREE_LOWER_BOUND_MACRO(T, N, int8_t, 100)        \
  BTREE_LOWER_BOUND_MACRO(T, N, int8_t, 200)        \
  BTREE_LOWER_BOUND_MACRO(T, N, int16_t, 100)       \
  BTREE_LOWER_BOUND_MACRO(T, N, int16_t, 1000)      \
  BTREE_LOWER_BOUND_MACRO(T, N, int16_t, 10000)     \
  BTREE_LOWER_BOUND_MACRO(T, N, int16_t, 50000)     \
  BTREE_LOWER_BOUND_MACRO(T, N, int32_t, 100)       \
  BTREE_LOWER_BOUND_MACRO(T, N, int32_t, 1000)      \
  BTREE_LOWER_BOUND_MACRO(T, N, int32_t, 10000)     \
  BTREE_LOWER_BOUND_MACRO(T, N, int32_t, 100000)    \
  BTREE_LOWER_BOUND_MACRO(T, N, int32_t, 1000000)   \
  BTREE_LOWER_BOUND_MACRO(T, N, int32_t, 10000000)  \
  BTREE_LOWER_BOUND_MACRO(T, N, int64_t, 100)       \
  BTREE_LOWER_BOUND_MACRO(T, N, int64_t, 1000)      \
  BTREE_LOWER_BOUND_MACRO(T, N, int64_t, 10000)     \
  BTREE_LOWER_BOUND_MACRO(T, N, int64_t, 100000)    \
  BTREE_LOWER_BOUND_MACRO(T, N, int64_t, 1000000)   \
  BTREE_LOWER_BOUND_MACRO(T, N, int64_t, 10000000)

  HWY_VISIT_TARGETS(RUN_ALL_BENCHMARKS);
  return 0;
}
#endif