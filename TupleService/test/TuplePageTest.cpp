#include "../domain/TuplePage.h"
#include <gtest/gtest.h>

TEST(TuplePageTest, MaintainsPageChainAndSkipsDeletedSlots) {
  TuplePage page;
  const auto first = page.insert({'a', 'b'});
  page.insert({'c'});
  page.remove(first);
  EXPECT_EQ(page.getAllTuples().size(), 1U);
  EXPECT_EQ(page.getAllTuples().front(), (std::vector<char>{'c'}));
  page.setNextPageId(17);
  EXPECT_EQ(page.getNextPageId(), 17U);
}
