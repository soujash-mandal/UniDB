#include "../../domain/TuplePage.h"
#include <gtest/gtest.h>

#include <vector>

// Scenario 1: Empty tuples remain distinguishable from deleted slots.
TEST(TuplePageTest, StoresEmptyTupleAndSkipsDeletedSlot) {
  TuplePage page;
  const auto emptySlot = page.insert({});
  const auto deletedSlot = page.insert({'x'});
  page.remove(deletedSlot);

  EXPECT_TRUE(page.get(emptySlot).empty());
  EXPECT_THROW(page.get(deletedSlot), std::runtime_error);
  const auto tuples = page.getAllTuples();
  ASSERT_EQ(tuples.size(), 1U);
  EXPECT_TRUE(tuples.front().empty());
}
