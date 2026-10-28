#include "DeallocatePageActionTestSupport.h"

#include <gtest/gtest.h>

TEST(DeallocatePageActionTest, RejectsReservedMetadataPage) {
  // Scenario 1: The first metadata page can never be returned to allocation.
  DeallocateMetadataStore store;
  store.pages.emplace(0, MetadataPage{});
  DeallocatePageAction action(store, store);

  EXPECT_THROW(action.execute(0), std::invalid_argument);
  EXPECT_EQ(store.writes, 0);
}
