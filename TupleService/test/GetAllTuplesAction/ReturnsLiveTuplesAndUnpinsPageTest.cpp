#include "GetAllTuplesActionTestSupport.h"

#include <gtest/gtest.h>

#include <vector>

// Scenario 1: Return live tuples in their original slot order and unpin the page.
TEST(GetAllTuplesActionTest, ReturnsLiveTuplesInSlotOrder) {
  GetAllTuplesFetchPort fetch;
  fetch.page.insert({'a'});
  fetch.page.insert({'b', 'c'});
  fetch.page.insert({'d'});
  GetAllTuplesUnpinPort unpin;
  GetAllTuplesAction action(fetch, unpin);

  EXPECT_EQ(action.execute(12),
            (std::vector<std::vector<char>>{{'a'}, {'b', 'c'}, {'d'}}));
  EXPECT_EQ(unpin.calls, 1);
  EXPECT_EQ(unpin.lastPageId, 12);
}
