#include "../../actions/CreateTableAction.h"
#include "../../actions/GetTableAction.h"
#include "../CatalogActionTestSupport.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

TEST(CreateTableActionTest, RejectsCyclicCatalogChainWithoutLeakingPins) {
  InMemoryCatalogPageStore store;
    CatalogPage page;
    page.setNextPageId(10);
    store.seed(10, page);
    CreateTableAction action(store, store, store, store);
    EXPECT_THROW(action.execute(10, 2, "users", {}), std::runtime_error);
    EXPECT_FALSE(store.hasPinnedPages());
}
