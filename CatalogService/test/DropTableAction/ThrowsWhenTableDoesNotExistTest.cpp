#include "../../actions/DropTableAction.h"
#include "../CatalogActionTestSupport.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

TEST(DropTableActionTest, ThrowsWhenTableDoesNotExist) {
InMemoryCatalogPageStore store;
  store.seed(10, CatalogPage{});
  DropTableAction action(store, store, store);
  EXPECT_THROW(action.execute(10, 99), std::runtime_error);
  EXPECT_EQ(store.unpinnedPageIds, (std::vector<CatalogPageId>{10}));
  EXPECT_FALSE(store.hasPinnedPages());
}
