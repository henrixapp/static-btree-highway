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

## Constructing the B+-Tree

We follow the instructions from the [algorithmica.org article](https://en.algorithmica.org/hpc/data-structures/s-tree/#construction-1) to construct a static, implicit, b+-tree.
Please refer to this article to better understand B+-Trees.

What we have to know here is, that SIMD instructions work on parallel on multiple data inputs. In Highway, each of these inputs is called a `lane`.
Similarly to the article, we want our code to use B=2* available Lanes per architecture (we will later also test for B=Lanes). This allows us to compare every entry in the node of the b-tree with two instructions.
The number of Lanes could change during runtime according to the highway doc in the future, but is currently not the case. For now, we assume that during runtime it does not change.

In Highway a [Tag](https://github.com/google/highway/blob/master/g3doc/quick_reference.md#vector-and-tag-types) is used to select the correct overloaded function. 

```cpp
// in static_btree/static_btree.hh
  static constexpr const hn::ScalableTag<ValueType> d{};
#if HWY_HAVE_CONSTEXPR_LANES
  static const HWY_LANES_CONSTEXPR size_t B = 2 * hn::Lanes(d);
#else
  const size_t B;
#endif
  explicit ImplicitStaticBTree(const std::vector<ValueType>& values)
#if !HWY_HAVE_CONSTEXPR_LANES
      : B(2 * hn::Lanes(d))
#endif
  {
  }
```

### Writing the helper functions
Before we can allocate the memory for our b-tree, we need to define some helper functions to calculate the height and size of the tree.
We use `HWY_LANES_CONSTEXPR` instead of `constexpr` to mark these functions, so that the compiler can optimize better, if constexpr is available.
Instead of the static constexpr functions used in the article, we use a std::vector to store the offsets.

The allocation of the btree is done via `hwy::AllocateAligned`, making sure that our array is aligned.
Since we later want to use Load (requiring the offsets to be a multiple of #Number of Lanes), we check that every offset is a multiple of `B` (which is a multiple of #Number of Lanes).  
```cpp
  HWY_LANES_CONSTEXPR size_t blocks(size_t n) { return (n + B - 1) / B; }
  HWY_LANES_CONSTEXPR size_t prev_keys(size_t n) { return (blocks(n) + B) / (B + 1) * B; }
  HWY_LANES_CONSTEXPR size_t height(size_t n) { return (n <= B ? 1 : height(prev_keys(n)) + 1); }
  const size_t N;
  const size_t H;
  size_t overall_size;
  std::vector<size_t> offsets;
  hwy::AlignedFreeUniquePtr<ValueType[]> btree;
  explicit ImplicitStaticBTree(const size_t N)
      : N(N),
        H(height(N))
#if !HWY_HAVE_CONSTEXPR_LANES
        ,
        B(2 * hn::Lanes(d))
#endif
  {
    offsets.resize(H + 1, 0);
    size_t k = 0, n = N;
    for (size_t i = 1; i < H; i++) {
      k += blocks(n) * B;  // every offset is multiple of B
      n = prev_keys(n);
      offsets[i] = k;
    }
    overall_size = k + B;
    offsets[H] = overall_size;
    btree = hwy::AllocateAligned<ValueType>(overall_size);
  }
```
### Building the implicit tree

The algorithm for building is simply the algorithm given by the article.

```cpp
 explicit ImplicitStaticBTree(const std::vector<ValueType>& values)
      : ImplicitStaticBTree(values.size()) {
    std::copy(values.begin(), values.end(), btree.get());
    build();
  }
  static const constexpr ValueType max_value = std::numeric_limits<ValueType>::max();
  void build() {
    // pad the array
    std::fill(btree.get() + N, btree.get() + overall_size, max_value);
    // build layer by layer
    for (size_t h = 1; h < H; h++) {
      for (size_t i = 0; i < offsets[h + 1] - offsets[h]; i++) {
        // i = k * B + j
        size_t k = (i / B), j = i - (k * B);
        k = k * (B + 1) + j + 1;  // compare to the right of the key
        // and then always to the left
        for (size_t l = 0; l < h - 1; l++) k *= (B + 1);
        // pad the rest with infinities if the key doesn't exist
        btree[offsets[h] + i] = ((k * B) < N ? btree[k * B] : max_value);
      }
    }
  }
```
We skip the permutation of the nodes as outlined in the article, since we later will see that this is a fine-grained improvement, not available on all plattforms in highway.

### Implementing the lower-bound

We can now write the logic needed to find the lower-bound for a value. We want to compare multiple values at the same time, so we populate a Vec `x` with the value we want to search for.
Notice how we use the `Tag` to use the right implementation.
The algorithmica implementation uses the greater-eq  and bit-tricks to find the lowerbound in every layer of the b-tree.
After consulting the [quick-reference](https://github.com/google/highway/blob/master/g3doc/quick_reference.md#test-mask), we replace this by a less-then comparison and counting 
how many entries are `true` in the mask.

In the reference there is also `FindFirstTrue` or `FindFirstFalse`, but this returns a `-1` if no value is found, which would need an extra branch to detect.
Feel free to change the implementation to `FindFirstFalse` with an extra if-statement and benchmark it.

```cpp
  size_t lower_bound(const ValueType val) {
    size_t k = 0;  // we assume k already multiplied by B to optimize pointer arithmetic
    auto x = hn::Set(d, _x);
    for (size_t h = H - 1; h > 0; h--) {
      auto i = hn::CountTrue(d, hn::Lt(hn::Load(d, btree.get() + offsets[h] + k), x)) +
               hn::CountTrue(d, hn::Lt(hn::Load(d, btree.get() + offsets[h] + k + B / 2), x));
      k = k * (B + 1) + (i * B);
    }
    auto i = hn::CountTrue(d, hn::Lt(hn::Load(d, btree.get() + k), x)) +
             hn::CountTrue(d, hn::Lt(hn::Load(d, btree.get() + k + B / 2), x));

    auto result = (k + i);
    return result;
  }
```

In order to test the results, we add a Benchmark to `static_btree/static_btree_benchmark.cc` and add the dependency to our benchmark:

```
# static_btree/BUILD
cc_binary(
    name = "static_btree_benchmark",
    srcs = ["static_btree_benchmark.cc"],
    visibility = ["//visibility:public"],
    deps = [
        ":static_btree",
        "@google_benchmark//:benchmark_main",
    ],
)
```

```cpp
//static_btree/static_btree_benchmark.cc
#include "static_btree/static_btree.hh"
//..
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
```

Now we can run the benchmark and see the results:

```
Run on (16 X 3293.83 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 1024 KiB (x8)
  L3 Unified 16384 KiB (x1)
Load Average: 0.42, 0.48, 0.34
-------------------------------------------------------------------------------------------------------------------------------------------------
Benchmark                                                                                       Time             CPU   Iterations UserCounters...
-------------------------------------------------------------------------------------------------------------------------------------------------
TreeLowerBound<henrixapp::static_btree::N_AVX3::ImplicitStaticBTree, uint32_t>/100            130 ns          133 ns      5092794 mask=20
TreeLowerBound<henrixapp::static_btree::N_AVX3::ImplicitStaticBTree, uint32_t>/1000          1295 ns         1327 ns       514982 mask=1.01k
TreeLowerBound<henrixapp::static_btree::N_AVX3::ImplicitStaticBTree, uint32_t>/10000        20176 ns        20672 ns        32612 mask=15.113k
TreeLowerBound<henrixapp::static_btree::N_AVX3::ImplicitStaticBTree, uint32_t>/1000000    3663054 ns      3753407 ns          200 mask=748.516k
StdLowerBound<uint32_t>/100                                                                   142 ns          146 ns      4736248 mask=20
StdLowerBound<uint32_t>/1000                                                                 5317 ns         2093 ns       336615 mask=1.01k
StdLowerBound<uint32_t>/10000                                                               32462 ns        33004 ns        21480 mask=15.113k
StdLowerBound<uint32_t>/1000000                                                          10796554 ns     10976693 ns           70 mask=748.516k
```
We can see that the masks are identical and we are also faster than std::lower_bound.
Furthermore,  our simple test passes (`bazel test -c dbg static_btree:static_btree_test --test_output=all`).