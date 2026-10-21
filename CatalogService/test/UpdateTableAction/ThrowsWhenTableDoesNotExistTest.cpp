#include "../../actions/UpdateTableAction.h"
#include "../CatalogActionTestSupport.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

TEST(UpdateTableActionTest, ThrowsWhenTableDoesNotExist) {
InMemoryCatalogPageStore store;
  store.seed(10, CatalogPage{});
  UpdateTableAction action(store, store, store);
  Table missing{};
  missing.tableId = 99;
  missing.name = "missing";

  EXPECT_THROW(action.execute(10, missing), std::runtime_error);
  EXPECT_EQ(store.unpinnedPageIds, (std::vector<CatalogPageId>{10}));
  EXPECT_FALSE(store.hasPinnedPages());
}
