#include "../../actions/GetTableAction.h"
#include "../CatalogActionTestSupport.h"

#include <gtest/gtest.h>

#include <stdexcept>

TEST(GetTableActionTest, RejectsCyclicCatalogChainWithoutLeakingPins) {
  InMemoryCatalogPageStore store;
CatalogPage page;
page.setNextPageId(10);
store.seed(10, page);
GetTableAction action(store, store);
EXPECT_THROW(action.execute(10, 5), std::runtime_error);
EXPECT_FALSE(store.hasPinnedPages());
}
