#include "../../../TupleService/actions/CreateTupleAction.h"
#include "../../../TupleService/actions/CreateTuplePageAction.h"
#include "../../../TupleService/actions/GetTupleAction.h"
#include "../../adapter/GetTupleAdapter.h"
#include "../StorageAdapterTestSupport.h"
#include <gtest/gtest.h>

TEST(GetTupleAdapterTest, DelegatesTupleLookup) {
  FakeTupleStore store;
  CreateTuplePageAction createPage(store, store, store);
  const auto page = createPage.execute();
  CreateTupleAction insert(store, store, store);
  const auto result = insert.execute(page.pageId, {'r', 'o', 'w'});
  GetTupleAction action(store, store);
  GetTupleAdapter adapter(action);
  EXPECT_EQ(adapter.getTuple(page.pageId, result.slotId),
            (std::vector<char>{'r', 'o', 'w'}));
}
