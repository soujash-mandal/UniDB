#include "../../actions/GetTableByNameAction.h"
#include "../CatalogActionTestSupport.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

TEST(GetTableByNameActionTest, RejectsEmptyAndUnknownNames) {
  InMemoryCatalogPageStore store;
    store.seed(10, CatalogPage{});
    GetTableByNameAction action(store, store);
  
    EXPECT_THROW(action.execute(10, ""), std::invalid_argument);
    EXPECT_THROW(action.execute(10, "missing"), std::runtime_error);
    EXPECT_EQ(store.unpinnedPageIds, (std::vector<CatalogPageId>{10}));
    EXPECT_FALSE(store.hasPinnedPages());
}
