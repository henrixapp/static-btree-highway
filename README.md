# Static B+-Tree with Google Highway

In this repository we implement a static B+-Tree as defined in [this algorithmica.org article](https://en.algorithmica.org/hpc/data-structures/s-tree/#b-tree-layout-1) using [Google Highway](https://github.com/google/highway).

##  Setup a repository & discover available hardware architectures on your device

We first start by initializing a bazel repository with sensible defaults (see .bazelversion,.gitignore, WORKSPACE).
We populate our MODULE.bazel with the dependencies needed:

```
bazel_dep(name = "googletest", version = "1.17.0.bcr.2")
bazel_dep(name = "google_benchmark", version = "1.9.5")
bazel_dep(name = "highway", version = "1.3.0")
```
The first two are optional, but since we want to test and benchmark our implementations they are highly recommended.

### Listing architectures

With this setup we can query which architectures are available via highway:

```sh
$ bazel run -c opt @highway//:list_targets                                                                 
...
Executing tests from @@highway+//:list_targets
-----------------------------------------------------------------------------
Compiler: GCC 1303
Config: emu128:0 scalar:0 static:0 all_attain:0 is_test:0
Have: constexpr_lanes:1 runtime_dispatch:1 auxv:1 f16 type:1/ops1 bf16 type:1/ops1
Compiled HWY_TARGETS:   AVX10_2 AVX3_SPR AVX3_ZEN4 AVX3_DL AVX3 AVX2 SSE4 SSSE3 SSE2
HWY_ATTAINABLE_TARGETS: AVX10_2 AVX3_SPR AVX3_ZEN4 AVX3_DL AVX3 AVX2 SSE4 SSSE3 SSE2 SCALAR
HWY_BASELINE_TARGETS:   SSE2 SCALAR
HWY_STATIC_TARGET:      SSE2
HWY_BROKEN_TARGETS:     LASX LSX
HWY_DISABLED_TARGETS:  
Current CPU supports:   AVX3_ZEN4 AVX3_DL AVX3 AVX2 SSE4 SSSE3 SSE2 EMU128 SCALAR
```

As we can see, there is a fairly modern CPU from AMD in my machine, supporting AVX3.

## Defining a baseline 

Before we start implementing our B+-Tree, we want to see how fast the binary search provided by std::lower_bound is.

We initialize a `static_btree/` directory with a `BUILD` file with the following content

```
cc_binary(
    name = "static_btree_benchmark",
    srcs = ["static_btree_benchmark.cc"],
    visibility = ["//visibility:public"],
    deps = [
        "@google_benchmark//:benchmark_main",
    ],
)
``` 

We add a simple benchmark file, which benchmarks std::lower_bound benchmarking on 100 to 10^6 points (in the vector) and retrieving the lower bound for 50 to 0.5e6 points:

```cpp
#include <algorithm>
#include <cstdint>

#include "benchmark/benchmark.h"

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
```

The results are as following:
```sh
$ bazel run -c opt //static_btree:static_btree_benchmark 
...
Run on (16 X 3293.83 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 1024 KiB (x8)
  L3 Unified 16384 KiB (x1)

Load Average: 0.16, 0.26, 0.36
------------------------------------------------------------------------------------------
Benchmark                                Time             CPU   Iterations UserCounters...
------------------------------------------------------------------------------------------
StdLowerBound<uint32_t>/100            135 ns          137 ns      5013407 mask=20
StdLowerBound<uint32_t>/1000          1953 ns         1974 ns       355643 mask=1.01k
StdLowerBound<uint32_t>/10000        49228 ns        31333 ns        22067 mask=15.113k
StdLowerBound<uint32_t>/1000000    8834368 ns      8936274 ns           77 mask=748.516k
```
The mask is used to ensure that the bounds are actually computed.

## Setting up the Highway skeleton & class
We now define our `cc_library` rule in `static_btree/BUILD` with the highway dependency:
```
cc_library(
    name = "static_btree",
    hdrs = ["static_btree.hh"],
    deps = ["@highway//:hwy"],
)
```

We init the static_btree.hh file with the harness [defined in the skeleton.cc](https://github.com/google/highway/blob/master/hwy/examples/skeleton.cc)
and define a dummy `ImplicitStaticBTree` struct, that has a constructor taking in values as a vector and a lowerbound method, to get the index of a lowerbound.
Note, that the current implementation is just made to get the code compile.
```cpp
//static_btree.hh
// notice that there is no #pragma once (would cause the reinclusion to fail)
#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "static_btree/static_btree.hh"
#include "hwy/foreach_target.h"  // IWYU pragma: keep

// This must be after the foreach_target include
#include "hwy/highway.h"

HWY_BEFORE_NAMESPACE();   // at file scope
namespace henrixapp {     // optional
namespace static_btree {  // optional
namespace HWY_NAMESPACE {
namespace hn = hwy::HWY_NAMESPACE;  // later used to get the correct functions for the current
                                        // architecture.
template <typename ValueType>
struct ImplicitStaticBTree {
  explicit ImplicitStaticBTree(const std::vector<ValueType>& values) {}
  size_t lower_bound(const ValueType val) { return 0; }
};
// NOLINTNEXTLINE(google-readability-namespace-comments)
}  // namespace HWY_NAMESPACE
}  // namespace static_btree
}  // namespace henrixapp
HWY_AFTER_NAMESPACE();
```

### Adding a simple test case
Before we continue we add a simple testcase (and a cc_test):

```python
# Add to static_btree/BUILD
cc_test(
    name = "static_btree_test",
    srcs = ["static_btree_test.cc"],
    deps = [
        ":static_btree",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
```

```cpp
// static_btree/static_btree_test.cc
#include "static_btree/static_btree.hh"

#include <gtest/gtest-matchers.h>
#include <gtest/gtest.h>
#include <sys/types.h>

TEST(StaticBTreeTest, BasicTests) {
  using VT = uint32_t;
  std::vector<VT> values{1, 10, 14, 28, 36};
  // We will later replace this and generate test for every architecture available
  henrixapp::static_btree::N_SSE2::ImplicitStaticBTree<VT> btree(values);
  ASSERT_EQ(btree.lower_bound(100), 5) << "Out of bound test failed";
  ASSERT_EQ(btree.lower_bound(0), 0) << "Smaller than value test failed";
}
```

Executing the test (assuming you have sse2 support on your CPU, that is almost guaranteed), you will get:

```sh
bazel test -c opt static_btree:static_btree_test --test_output=all
...
Running main() from gmock_main.cc
[==========] Running 1 test from 1 test suite.
[----------] Global test environment set-up.
[----------] 1 test from StaticBTreeTest
[ RUN      ] StaticBTreeTest.BasicTests
static_btree/static_btree_test.cc:12: Failure
Expected equality of these values:
  btree.lower_bound(100)
    Which is: 0
  5
Out of bound test failed

[  FAILED  ] StaticBTreeTest.BasicTests (0 ms)
[----------] 1 test from StaticBTreeTest (0 ms total)

[----------] Global test environment tear-down
[==========] 1 test from 1 test suite ran. (0 ms total)
[  PASSED  ] 0 tests.
[  FAILED  ] 1 test, listed below:
[  FAILED  ] StaticBTreeTest.BasicTests

 1 FAILED TEST
...
```
