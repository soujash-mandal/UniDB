#include "DeallocatePageActionTestSupport.h"

#include <gtest/gtest.h>

TEST(DeallocatePageActionTest, FreesAllocatedPageForReuse) {
  // Scenario 1: A reclaimed allocation becomes the next available page ID.
  DeallocateMetadataStore store;
  MetadataPage metadata;
  metadata.setPageOccupied(1);
  store.pages.emplace(0, metadata);
  DeallocatePageAction action(store, store);

  action.execute(1);

  EXPECT_EQ(store.pages.at(0).findFirstFreePage(), 1);
  EXPECT_EQ(store.writes, 1);
}
