#include "../../actions/CreateTableAction.h"
#include "../../actions/GetTableAction.h"
#include "../CatalogActionTestSupport.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

TEST(CreateTableActionTest, RejectsEmptyTableName) {
  InMemoryCatalogPageStore store;
    store.seed(10, CatalogPage{});
    CreateTableAction action(store, store, store, store);
    EXPECT_THROW(action.execute(10, 1, "", {}), std::invalid_argument);
    EXPECT_FALSE(store.hasPinnedPages());
}
