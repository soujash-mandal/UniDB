#include "../../actions/CreateTableAction.h"
#include "../../actions/GetTableAction.h"
#include "../CatalogActionTestSupport.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

TEST(CreateTableActionTest, RejectsEmptyNameAndDuplicateName) {
  InMemoryCatalogPageStore store;
    store.seed(10, CatalogPage{});
    CreateTableAction action(store, store, store, store);
  
    EXPECT_THROW(action.execute(10, 1, "", {}), std::invalid_argument);
    action.execute(10, 1, "users", {});
    EXPECT_THROW(action.execute(10, 2, "users", {}), std::runtime_error);
    EXPECT_FALSE(store.hasPinnedPages());
}
