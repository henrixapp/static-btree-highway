#include <cassert>
#include <cstddef>
#include <iostream>
#include <limits>

#include "hwy/aligned_allocator.h"
#include "hwy/highway.h"
HWY_BEFORE_NAMESPACE();   // at file scope
namespace henrixapp {     // optional
namespace static_btree {  // optional
namespace HWY_NAMESPACE {
namespace hn = hwy::HWY_NAMESPACE;  // later used to get the correct functions for the current
                                    // architecture.
template <typename ValueType>
struct ImplicitStaticBTree {
  using DataType = ValueType;
  static constexpr const hn::ScalableTag<ValueType> d{};
#if HWY_HAVE_CONSTEXPR_LANES
  static const HWY_LANES_CONSTEXPR size_t B = 2 * hn::Lanes(d);
#else
  const size_t B;
#endif
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
  size_t lower_bound(const ValueType _x) {
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
};
// NOLINTNEXTLINE(google-readability-namespace-comments)
}  // namespace HWY_NAMESPACE
}  // namespace static_btree
}  // namespace henrixapp
HWY_AFTER_NAMESPACE();