#include "GetAllTuplesActionTestSupport.h"

#include <gtest/gtest.h>

// Scenario 3: Return an empty collection and release the fetched page.
TEST(GetAllTuplesActionTest, ReturnsEmptyForEmptyPage) {
  GetAllTuplesFetchPort fetch;
  GetAllTuplesUnpinPort unpin;
  GetAllTuplesAction action(fetch, unpin);

  EXPECT_TRUE(action.execute(14).empty());
  EXPECT_EQ(unpin.calls, 1);
  EXPECT_EQ(unpin.lastPageId, 14);
}
