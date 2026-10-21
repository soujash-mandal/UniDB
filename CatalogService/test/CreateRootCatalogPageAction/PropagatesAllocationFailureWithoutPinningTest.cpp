#include "../../actions/CreateRootCatalogPageAction.h"
#include "../CatalogActionTestSupport.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

TEST(CreateRootCatalogPageActionTest, PropagatesAllocationFailureWithoutPinning) {
  InMemoryCatalogPageStore store;
  store.failNextAllocation = true;
  CreateRootCatalogPageAction action(store, store, store);
  EXPECT_THROW(action.execute(), std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}
