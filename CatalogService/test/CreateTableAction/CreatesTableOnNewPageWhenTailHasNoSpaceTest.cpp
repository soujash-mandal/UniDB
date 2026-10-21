#include "../../actions/CreateTableAction.h"
#include "../../actions/GetTableAction.h"
#include "../CatalogActionTestSupport.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

TEST(CreateTableActionTest, CreatesTableOnNewPageWhenTailHasNoSpace) {
  InMemoryCatalogPageStore store;
    store.seed(10, makeCatalogPageWithoutRoomForAnotherTable());
    CreateTableAction action(store, store, store, store);
    EXPECT_EQ(action.execute(10, 999, "users", {}), 999U);
    GetTableAction getTable(store, store);
    EXPECT_EQ(getTable.execute(10, 999).name, "users");
    EXPECT_EQ(store.pages.at(10).getNextPageId(), 11U);
    EXPECT_FALSE(store.hasPinnedPages());
}
