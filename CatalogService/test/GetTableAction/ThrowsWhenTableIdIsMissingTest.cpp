#include "../../actions/GetTableAction.h"
#include "../CatalogActionTestSupport.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

TEST(GetTableActionTest, ThrowsWhenTableIdIsMissing) {
  InMemoryCatalogPageStore store;
    store.seed(10, CatalogPage{});
    GetTableAction action(store, store);
    EXPECT_THROW(action.execute(10, 404), std::runtime_error);
    EXPECT_EQ(store.unpinnedPageIds, (std::vector<CatalogPageId>{10}));
    EXPECT_FALSE(store.hasPinnedPages());
}
