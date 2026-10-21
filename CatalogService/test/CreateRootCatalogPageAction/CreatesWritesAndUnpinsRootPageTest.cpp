#include "../../actions/CreateRootCatalogPageAction.h"
#include "../CatalogActionTestSupport.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

TEST(CreateRootCatalogPageActionTest, CreatesWritesAndUnpinsRootPage) {
  InMemoryCatalogPageStore store;
  CreateRootCatalogPageAction action(store, store, store);
  const CatalogPageId rootPageId = action.execute();
  EXPECT_NE(rootPageId, CATALOG_INVALID_PAGE_ID);
  EXPECT_TRUE(store.pages.contains(rootPageId));
  EXPECT_EQ(store.writtenPageIds, (std::vector<CatalogPageId>{rootPageId}));
  EXPECT_EQ(store.unpinnedPageIds, (std::vector<CatalogPageId>{rootPageId}));
  EXPECT_FALSE(store.hasPinnedPages());
}
