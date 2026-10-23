#include "../../actions/GetTupleAction.h"
#include "../TupleActionTestSupport.h"
#include <gtest/gtest.h>

#include <stdexcept>

// Scenario 1: Reject deleted tuple slots and unpin the page.
TEST(GetTupleActionTest, RejectsDeletedSlotAndUnpinsPage) {
  InMemoryTuplePageStore store;
  TuplePage page;
  page.insert({'x'});
  page.remove(0);
  store.seed(11, page);
  GetTupleAction action(store, store);

  EXPECT_THROW(action.execute(11, 0), std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}
