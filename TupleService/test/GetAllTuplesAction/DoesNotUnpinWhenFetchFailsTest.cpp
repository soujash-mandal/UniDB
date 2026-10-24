#include "GetAllTuplesActionTestSupport.h"

#include <gtest/gtest.h>

#include <stdexcept>

// Scenario 4: Do not unpin a page when fetching it failed before acquisition.
TEST(GetAllTuplesActionTest, DoesNotUnpinWhenFetchFails) {
  GetAllTuplesFetchPort fetch;
  fetch.failFetch = true;
  GetAllTuplesUnpinPort unpin;
  GetAllTuplesAction action(fetch, unpin);

  EXPECT_THROW(action.execute(15), std::runtime_error);
  EXPECT_EQ(unpin.calls, 0);
}
