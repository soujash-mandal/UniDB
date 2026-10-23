#include "../../domain/TuplePage.h"
#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

// Scenario 1: Tuple pages use full page capacity and omit deleted slots.
TEST(TuplePageTest, UsesFullPageCapacityAndSkipsDeletedSlots) {
  TuplePage page;
  EXPECT_EQ(page.getFreeSpace(), TUPLE_PAGE_SIZE - sizeof(uint32_t) * 2);
  const auto first = page.insert({'a', 'b'});
  page.insert({'c'});
  page.remove(first);
  EXPECT_EQ(page.getAllTuples().size(), 1U);
  EXPECT_EQ(page.getAllTuples().front(), (std::vector<char>{'c'}));
}
