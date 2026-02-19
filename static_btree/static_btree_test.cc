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