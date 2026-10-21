#include "../../actions/UpdateTableAction.h"
#include "../CatalogActionTestSupport.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

TEST(UpdateTableActionTest, RejectsNameUsedByDifferentTableOnAnotherPage) {
  InMemoryCatalogPageStore store;
  CatalogPage root;
  root.setNextPageId(11);
  Table target{};
  target.tableId = 2;
  target.name = "orders";
  root.insert(target);

  CatalogPage second;
  Table existing{};
  existing.tableId = 1;
  existing.name = "users";
  second.insert(existing);

  store.seed(10, root);
  store.seed(11, second);
  UpdateTableAction action(store, store, store, store);
  target.name = "users";

  EXPECT_THROW(action.execute(10, target), std::runtime_error);
  EXPECT_TRUE(store.writtenPageIds.empty());
  EXPECT_EQ(store.unpinnedPageIds, (std::vector<CatalogPageId>{10, 11}));
  EXPECT_FALSE(store.hasPinnedPages());
}
