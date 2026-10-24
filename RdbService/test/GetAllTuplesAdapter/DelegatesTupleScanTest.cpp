#include "../../adapter/GetAllTuplesAdapter.h"
#include "../StorageAdapterTestSupport.h"
#include "../../../TupleService/actions/CreateTupleAction.h"
#include "../../../TupleService/actions/CreateTuplePageAction.h"
#include "../../../TupleService/actions/GetAllTuplesAction.h"
#include <gtest/gtest.h>
TEST(GetAllTuplesAdapterTest, DelegatesTupleScan) {
  FakeTupleStore store;
  CreateTuplePageAction createPage(store, store, store);
  const auto page = createPage.execute();
  CreateTupleAction insert(store, store, store);
  const std::vector<char> row{'r', 'o', 'w'};
  insert.execute(page.pageId, row);
  GetAllTuplesAction action(store, store);
  GetAllTuplesAdapter adapter(action);
  EXPECT_EQ(adapter.getAllTuples(page.pageId), (std::vector<std::vector<char>>{row}));
}
