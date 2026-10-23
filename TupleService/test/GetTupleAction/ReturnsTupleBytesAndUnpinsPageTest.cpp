#include "../../actions/GetTupleAction.h"
#include "../TupleActionTestSupport.h"
#include <gtest/gtest.h>

#include <vector>

// Scenario 1: Return tuple bytes and unpin the fetched page.
TEST(GetTupleActionTest, ReturnsTupleBytesAndUnpinsPage) {
  InMemoryTuplePageStore store;
  TuplePage page;
  page.insert({'o', 'k'});
  store.seed(11, page);
  GetTupleAction action(store, store);

  EXPECT_EQ(action.execute(11, 0), (std::vector<char>{'o', 'k'}));
  EXPECT_FALSE(store.hasPinnedPages());
}
