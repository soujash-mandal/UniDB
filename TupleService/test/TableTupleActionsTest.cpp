#include "../actions/InsertTupleIntoTableAction.h"
#include "../actions/ScanTupleTableAction.h"

#include <gtest/gtest.h>

#include <limits>
#include <unordered_map>

namespace {
class InMemoryTupleStore : public TupleFetchPagePort,
                           public TupleNewPagePort,
                           public TupleWritePagePort,
                           public TupleUnpinPagePort {
public:
  TuplePage fetchPage(TuplePageId pageId) override {
    ++pins[pageId];
    return pages.at(pageId);
  }

  TuplePageId newPage() override {
    const TuplePageId pageId = nextPageId++;
    pages.emplace(pageId, TuplePage{});
    ++pins[pageId];
    return pageId;
  }

  void writePage(TuplePageId pageId, TuplePage page) override {
    pages.insert_or_assign(pageId, page);
  }

  void unpinPage(TuplePageId pageId) override {
    auto it = pins.find(pageId);
    if (it == pins.end() || it->second == 0) {
      throw std::runtime_error("Unpin without matching pin");
    }
    --it->second;
  }

  bool hasPinnedPages() const {
    for (const auto &entry : pins) {
      if (entry.second != 0) {
        return true;
      }
    }
    return false;
  }

private:
  TuplePageId nextPageId{1};
  std::unordered_map<TuplePageId, TuplePage> pages;
  std::unordered_map<TuplePageId, uint32_t> pins;
};
} // namespace

TEST(TableTupleActionsTest, InsertsAcrossPagesAndScansRowsInOrder) {
  InMemoryTupleStore store;
  InsertTupleIntoTableAction insert(store, store, store, store);
  ScanTupleTableAction scan(store, store);

  const std::vector<char> firstRow(6000, 'a');
  const std::vector<char> secondRow(2500, 'b');
  const auto first =
      insert.execute(std::numeric_limits<TuplePageId>::max(), firstRow);
  const auto second = insert.execute(first.firstPageId, secondRow);

  EXPECT_EQ(first.firstPageId, first.pageId);
  EXPECT_NE(first.pageId, second.pageId);
  EXPECT_EQ(second.firstPageId, first.firstPageId);

  const auto rows = scan.execute(first.firstPageId);
  ASSERT_EQ(rows.size(), 2U);
  EXPECT_EQ(rows[0], firstRow);
  EXPECT_EQ(rows[1], secondRow);
  EXPECT_FALSE(store.hasPinnedPages());
}
