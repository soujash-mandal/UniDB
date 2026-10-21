#include "../../actions/GetTableByNameAction.h"
#include "../CatalogActionTestSupport.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

TEST(GetTableByNameActionTest, PropagatesFetchFailureWithoutPinning) {
  InMemoryCatalogPageStore store;
  store.failNextFetch = true;
  GetTableByNameAction action(store, store);

  EXPECT_THROW(action.execute(10, "products"), std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}
