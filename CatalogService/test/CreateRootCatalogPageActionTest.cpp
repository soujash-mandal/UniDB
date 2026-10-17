#include "../actions/CreateRootCatalogPageAction.h"
#include "CatalogActionTestSupport.h"

#include <gtest/gtest.h>

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

TEST(CreateRootCatalogPageActionTest, UnpinsAllocatedPageWhenWriteFails) {
  InMemoryCatalogPageStore store;
  store.failNextWrite = true;
  CreateRootCatalogPageAction action(store, store, store);

  EXPECT_THROW(action.execute(), std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}
