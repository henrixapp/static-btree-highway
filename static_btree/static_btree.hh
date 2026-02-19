
// Based on
// https://github.com/google/highway/blob/322d55670ca88d49e2565cd72757c5139b2b9fc1/hwy/examples/skeleton.cc
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