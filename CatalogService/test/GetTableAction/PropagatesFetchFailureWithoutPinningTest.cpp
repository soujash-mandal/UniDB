#include "../../actions/GetTableAction.h"
#include "../CatalogActionTestSupport.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

TEST(GetTableActionTest, PropagatesFetchFailureWithoutPinning) {
InMemoryCatalogPageStore store;
  store.failNextFetch = true;
  GetTableAction action(store, store);
  EXPECT_THROW(action.execute(10, 5), std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}
