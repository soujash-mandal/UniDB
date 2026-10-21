#include "../../actions/DropTableAction.h"
#include "../CatalogActionTestSupport.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

TEST(DropTableActionTest, RejectsCyclicCatalogChainWithoutLeakingPins) {
InMemoryCatalogPageStore store;
  CatalogPage page;
  page.setNextPageId(10);
  store.seed(10, page);
  DropTableAction action(store, store, store);
  EXPECT_THROW(action.execute(10, 99), std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}
