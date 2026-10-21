#include "../../actions/CreateTableAction.h"
#include "../../actions/GetTableAction.h"
#include "../CatalogActionTestSupport.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

TEST(CreateTableActionTest, ReleasesTailPageWhenAllocationFails) {
InMemoryCatalogPageStore store;
  store.seed(10, makeCatalogPageWithoutRoomForAnotherTable());
  store.failNextAllocation = true;
  CreateTableAction action(store, store, store, store);
  EXPECT_THROW(action.execute(10, 999, "a-new-table", {}), std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}
