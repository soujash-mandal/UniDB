#include "GetAllTuplesActionTestSupport.h"

#include <gtest/gtest.h>

#include <stdexcept>

// Scenario 6: Propagate unpin failure rather than returning an apparently successful scan.
TEST(GetAllTuplesActionTest, PropagatesUnpinFailure) {
  GetAllTuplesFetchPort fetch;
  fetch.page.insert({'x'});
  GetAllTuplesUnpinPort unpin;
  unpin.failUnpin = true;
  GetAllTuplesAction action(fetch, unpin);

  EXPECT_THROW(action.execute(17), std::runtime_error);
  EXPECT_EQ(unpin.calls, 1);
  EXPECT_EQ(unpin.lastPageId, 17);
}
