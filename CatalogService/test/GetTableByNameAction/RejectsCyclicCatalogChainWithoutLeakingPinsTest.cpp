#include "../../actions/GetTableByNameAction.h"
#include "../CatalogActionTestSupport.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

TEST(GetTableByNameActionTest, RejectsCyclicCatalogChainWithoutLeakingPins) {
  InMemoryCatalogPageStore store;
  CatalogPage page;
  page.setNextPageId(10);
  store.seed(10, page);
  GetTableByNameAction action(store, store);

  EXPECT_THROW(action.execute(10, "products"), std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}
