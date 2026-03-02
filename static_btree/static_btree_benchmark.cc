
#include <sys/types.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

#include "static_btree/benchmark_helpers.hh"
#undef HWY_TARGET_INCLUDE
// For dynamic dispatch, specify the name of the current file (unfortunately
// __FILE__ is not reliable) so that foreach_target.h can re-include it.
#define HWY_TARGET_INCLUDE "static_btree/static_btree_benchmark.cc"
// Generates code for each enabled target by re-including this source file.

#include "hwy/foreach_target.h"  // IWYU pragma: keep
#include "hwy/highway.h"
#include "hwy/nanobenchmark.h"
#include "hwy/tests/test_util-inl.h"
#include "static_btree/benchmark_helpers.hh"
#include "static_btree/static_btree.hh"

namespace static_btree_bench {

namespace HWY_NAMESPACE {
namespace {
// experiment config
static constexpr const int query_sets = 1;
static constexpr const int queries_per_sets = 10000;
static constexpr const int start_value_experiment = 32;
static constexpr const int multiplier_experiment = 2;
static constexpr const int stop_value_experiment = 1 << 26;

template <class LowerBoundable>
struct Benchmark {
  using DataType = typename LowerBoundable::DataType;
  LowerBoundable instance;
  std::vector<std::vector<DataType>> queries;
  size_t mask = 0;
  Benchmark(const std::vector<DataType>& inputs, const std::vector<std::vector<DataType>>& queries)
      : instance(inputs), queries(queries) {}
  size_t operator()(size_t i) {
    size_t Mask = 0;
    for (auto p : queries[i]) {
      Mask ^= instance.lower_bound(p);
    }
    mask = Mask;
    return Mask;
  }
};
template <typename DS, int n_queries, int num_per_query>
void RunBench(const std::string& name, size_t n_inputs) {
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

  size_t result_count = 0;

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
  std::cout << name << "," << ticks / n_queries << "," << var / n_queries << "," << benchmark.mask
            << std::endl;
}
}  // namespace
namespace hn = hwy::HWY_NAMESPACE;
struct BenchmarkSuite {
  template <typename DT>
  void operator()(DT) const {
    using BTree = henrixapp::static_btree::HWY_NAMESPACE::ImplicitStaticBTree<DT>;
    auto info = hwy::detail::MakeTypeInfo<DT>();
    char type_name[100];
    hwy::detail::TypeName(info, 1, type_name);
    for (size_t i = start_value_experiment; i < std::numeric_limits<DT>::max();
         i *= multiplier_experiment) {
      if (i > stop_value_experiment) {
        break;
      }
      RunBench<BTree, query_sets, queries_per_sets>(
          std::string(hwy::TargetName(HWY_TARGET)) + "," + std::to_string(BTree({}).B) + "," +
              std::to_string(i) + "," + std::string(type_name),
          i);
    }
  }
};
struct BenchmarkSuite1 {
  template <typename DT>
  void operator()(DT) const {
    using BTree = henrixapp::static_btree::HWY_NAMESPACE::ImplicitStaticBTree1<DT>;
    auto info = hwy::detail::MakeTypeInfo<DT>();
    char type_name[100];
    hwy::detail::TypeName(info, 1, type_name);
    for (size_t i = start_value_experiment; i < std::numeric_limits<DT>::max();
         i *= multiplier_experiment) {
      if (i > stop_value_experiment) {
        break;
      }
      RunBench<BTree, query_sets, queries_per_sets>(
          std::string(hwy::TargetName(HWY_TARGET)) + "," + std::to_string(BTree({}).B) + "," +
              std::to_string(i) + "," + std::string(type_name),
          i);
    }
  }
};
template <typename DT>
struct StdLowerbounder {
  using DataType = DT;
  const std::vector<DataType> data;
  StdLowerbounder(const std::vector<DataType>& _data) : data(_data) {}
  size_t lower_bound(DT val) {
    return std::lower_bound(data.begin(), data.end(), val) - data.begin();
  }
};
struct StdLowerboundSuite {
  template <typename DT>
  void operator()(DT) const {
    using STLLowerbounder = StdLowerbounder<DT>;
    auto info = hwy::detail::MakeTypeInfo<DT>();
    char type_name[100];
    hwy::detail::TypeName(info, 1, type_name);
    for (size_t i = start_value_experiment; i < std::numeric_limits<DT>::max();
         i *= multiplier_experiment) {
      if (i > stop_value_experiment) {
        break;
      }
      RunBench<STLLowerbounder, query_sets, queries_per_sets>(
          std::string("STL,1,") + std::to_string(i) + "," + std::string(type_name), i);
    }
  }
};
void RunBenchmark() { hn::ForAllTypes(BenchmarkSuite()); }
void RunBenchmark1() { hn::ForAllTypes(BenchmarkSuite1()); }

void RunStdLowerboundBenchmark() { hn::ForAllTypes(StdLowerboundSuite()); }
}  // namespace HWY_NAMESPACE
}  // namespace static_btree_bench
#if HWY_ONCE
namespace std {
// This is kind of nasty to do, but makes the other things compile more easily.
template <>
struct is_floating_point<hwy::float16_t> : std::true_type {};

template <>
struct numeric_limits<hwy::float16_t> {
  static constexpr hwy::float16_t max() { return hwy::HighestValue<hwy::float16_t>(); }
  static constexpr hwy::float16_t min() { return hwy::LowestValue<hwy::float16_t>(); }
};
}  // namespace std
namespace static_btree_bench {
HWY_EXPORT(RunBenchmark);
HWY_EXPORT(RunBenchmark1);
HWY_EXPORT(RunStdLowerboundBenchmark);
void RunBenchmarks() {
  HWY_DYNAMIC_DISPATCH(RunStdLowerboundBenchmark)();
  for (int64_t target : hwy::SupportedAndGeneratedTargets()) {
    hwy::SetSupportedTargetsForTest(target);
    HWY_DYNAMIC_DISPATCH(RunBenchmark)();
    HWY_DYNAMIC_DISPATCH(RunBenchmark1)();
  }
  hwy::SetSupportedTargetsForTest(0);  // Reset the mask afterwards.
}
}  // namespace static_btree_bench
int main() {
  std::cout << "ISA,B,N,Type,cpu_mean,var,mask" << std::endl;
  static_btree_bench::RunBenchmarks();
  return 0;
}
#endif