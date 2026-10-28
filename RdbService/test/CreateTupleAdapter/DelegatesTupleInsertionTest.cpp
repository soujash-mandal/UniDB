#include "../../../TupleService/actions/CreateTupleAction.h"
#include "../../../TupleService/actions/CreateTuplePageAction.h"
#include "../../adapter/CreateTupleAdapter.h"
#include "../StorageAdapterTestSupport.h"
#include <gtest/gtest.h>

TEST(CreateTupleAdapterTest, DelegatesTupleInsertion) {
  FakeTupleStore store;
  CreateTuplePageAction createPage(store, store, store);
  const auto page = createPage.execute();
  CreateTupleAction action(store, store, store);
  CreateTupleAdapter adapter(action);

  const auto result = adapter.createTuple(page.pageId, {'r', 'o', 'w'});
  EXPECT_EQ(result.slotId, 0U);
  EXPECT_EQ(result.freeSpace, page.freeSpace - 11U);
}
