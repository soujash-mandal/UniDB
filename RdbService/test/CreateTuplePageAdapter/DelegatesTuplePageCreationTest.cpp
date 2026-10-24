#include "../../adapter/CreateTuplePageAdapter.h"
#include "../StorageAdapterTestSupport.h"
#include "../../../TupleService/actions/CreateTuplePageAction.h"
#include <gtest/gtest.h>
TEST(CreateTuplePageAdapterTest, DelegatesTuplePageCreation) {
  FakeTupleStore store;
  CreateTuplePageAction action(store, store, store);
  CreateTuplePageAdapter adapter(action);
  const auto result = adapter.createTuplePage();
  EXPECT_EQ(result.pageId, 50U);
  EXPECT_GT(result.freeSpace, 0U);
}
