#include "DeallocatePageActionTestSupport.h"

#include <gtest/gtest.h>

TEST(DeallocatePageActionTest, RejectsDoubleFree) {
  // Scenario 1: A page already marked free cannot be deallocated twice.
  DeallocateMetadataStore store;
  store.pages.emplace(0, MetadataPage{});
  DeallocatePageAction action(store, store);

  EXPECT_THROW(action.execute(1), std::invalid_argument);
  EXPECT_EQ(store.writes, 0);
}
