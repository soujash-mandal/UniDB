#include "GetAllTuplesActionTestSupport.h"

#include <gtest/gtest.h>

#include <vector>

// Scenario 2: Omit deleted slots while preserving the order of live tuples.
TEST(GetAllTuplesActionTest, SkipsDeletedTuples) {
  GetAllTuplesFetchPort fetch;
  fetch.page.insert({'a'});
  fetch.page.insert({'b'});
  fetch.page.insert({'c'});
  fetch.page.remove(1);
  GetAllTuplesUnpinPort unpin;
  GetAllTuplesAction action(fetch, unpin);

  EXPECT_EQ(action.execute(13), (std::vector<std::vector<char>>{{'a'}, {'c'}}));
  EXPECT_EQ(unpin.calls, 1);
  EXPECT_EQ(unpin.lastPageId, 13);
}
