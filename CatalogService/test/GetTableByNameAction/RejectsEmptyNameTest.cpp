#include "../../actions/GetTableByNameAction.h"
#include "../CatalogActionTestSupport.h"

#include <gtest/gtest.h>

#include <stdexcept>

TEST(GetTableByNameActionTest, RejectsEmptyName) {
InMemoryCatalogPageStore store;
store.seed(10, CatalogPage{});
GetTableByNameAction action(store, store);
EXPECT_THROW(action.execute(10, ""), std::invalid_argument);
EXPECT_TRUE(store.unpinnedPageIds.empty());
EXPECT_FALSE(store.hasPinnedPages());
}
