#include <vector>

#include "hwy/tests/hwy_gtest.h"
// clang-format off
#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "static_btree/static_btree_test.cc"
#include "hwy/foreach_target.h"  // IWYU pragma: keep
#include "hwy/highway.h"
#include "hwy/tests/test_util-inl.h"
// clang-format on
#include "static_btree/static_btree.hh"

HWY_BEFORE_NAMESPACE();
namespace static_btree_test {
namespace HWY_NAMESPACE {
namespace hn = hwy::HWY_NAMESPACE;
struct BasicTests {
  template <class VT>
  void operator()(VT) const {
    std::vector<VT> values{1, 10, 14, 28, 36};
    // We will later replace this and generate test for every architecture available
    henrixapp::static_btree::HWY_NAMESPACE::ImplicitStaticBTree<VT> btree(values);
    HWY_ASSERT_EQ(5, btree.lower_bound(100));
    HWY_ASSERT_EQ(0, btree.lower_bound(0));
    HWY_ASSERT_EQ(0, btree.lower_bound(1));
  }
};
void TestAllBTree() { hn::ForAllTypes(BasicTests()); }
}  // namespace HWY_NAMESPACE
}  // namespace static_btree_test
HWY_AFTER_NAMESPACE();
#if HWY_ONCE
namespace static_btree_test {
namespace {
HWY_BEFORE_TEST(BTreeTest);
HWY_EXPORT_AND_TEST_P(BTreeTest, TestAllBTree);
HWY_AFTER_TEST();
}  // namespace
}  // namespace static_btree_test
HWY_TEST_MAIN();
#endif  // HWY_ONCE