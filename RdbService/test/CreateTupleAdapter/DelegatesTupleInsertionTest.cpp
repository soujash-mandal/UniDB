#include "../../adapter/CreateTupleAdapter.h"
#include "../StorageAdapterTestSupport.h"
#include "../../../TupleService/actions/CreateTupleAction.h"
#include "../../../TupleService/actions/CreateTuplePageAction.h"
#include <gtest/gtest.h>
TEST(CreateTupleAdapterTest, DelegatesTupleInsertion) {
  FakeTupleStore store;
  CreateTuplePageAction createPage(store, store, store);
  const auto page = createPage.execute();
  CreateTupleAction action(store, store, store);
  CreateTupleAdapter adapter(action);
  EXPECT_EQ(adapter.createTuple(page.pageId, {'r', 'o', 'w'}), 0U);
}
