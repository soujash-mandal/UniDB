#include "../../adapter/TuplePageAdapter.h"

#include "../../../TupleService/actions/CreateTupleAction.h"
#include "../../../TupleService/actions/CreateTuplePageAction.h"
#include "../../../TupleService/actions/GetAllTuplesAction.h"
#include "../../../TupleService/actions/GetTupleAction.h"
#include "../../../TupleService/domain/TuplePage.h"
#include "../../../TupleService/port/FetchPagePort.h"
#include "../../../TupleService/port/NewPagePort.h"
#include "../../../TupleService/port/UnpinPagePort.h"
#include "../../../TupleService/port/WritePagePort.h"

#include <gtest/gtest.h>

#include <map>
#include <stdexcept>
#include <vector>

namespace {
class FakeTupleStore final : public TupleNewPagePort,
                             public TupleWritePagePort,
                             public TupleUnpinPagePort,
                             public TupleFetchPagePort {
public:
  TuplePageId newPage() override {
    const auto id = nextPageId++;
    pages.emplace(id, TuplePage{});
    return id;
  }

  void writePage(TuplePageId pageId, TuplePage page) override {
    pages[pageId] = page;
  }

  void unpinPage(TuplePageId pageId) override { unpinned.push_back(pageId); }

  TuplePage fetchPage(TuplePageId pageId) override {
    const auto it = pages.find(pageId);
    if (it == pages.end()) {
      throw std::runtime_error("Tuple page not found");
    }
    return it->second;
  }

  std::map<TuplePageId, TuplePage> pages;
  std::vector<TuplePageId> unpinned;

private:
  TuplePageId nextPageId = 50;
};
} // namespace

TEST(TuplePageAdapterTest, DelegatesTuplePageAndRowOperations) {
  FakeTupleStore store;
  CreateTuplePageAction createPageAction(store, store, store);
  CreateTupleAction createTupleAction(store, store, store);
  GetTupleAction getTupleAction(store, store);
  GetAllTuplesAction getAllTuplesAction(store, store);
  TuplePageAdapter adapter(createPageAction, createTupleAction, getTupleAction,
                           getAllTuplesAction);

  const auto page = adapter.createTuplePage();
  const std::vector<char> row{'r', 'o', 'w'};
  const auto slotId = adapter.createTuple(page.pageId, row);

  EXPECT_EQ(page.pageId, 50U);
  EXPECT_EQ(adapter.getTuple(page.pageId, slotId), row);
  EXPECT_EQ(adapter.getAllTuples(page.pageId),
            (std::vector<std::vector<char>>{row}));
  EXPECT_FALSE(store.unpinned.empty());
}
