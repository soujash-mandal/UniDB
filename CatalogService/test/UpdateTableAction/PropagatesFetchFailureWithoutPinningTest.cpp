#include "../../actions/UpdateTableAction.h"
#include "../CatalogActionTestSupport.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

TEST(UpdateTableActionTest, PropagatesFetchFailureWithoutPinning) {
  InMemoryCatalogPageStore store;
    store.failNextFetch = true;
    UpdateTableAction action(store, store, store);
    Table table{};
    table.tableId = 9;
    table.name = "events";
  
    EXPECT_THROW(action.execute(10, table), std::runtime_error);
    EXPECT_FALSE(store.hasPinnedPages());
}
