#include "../actions/InsertTupleIntoTableAction.h"
#include "../actions/ScanTupleTableAction.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
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
    if (failNextAllocation) {
      failNextAllocation = false;
      throw std::runtime_error("Injected page allocation failure");
    }
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

  void failNextPageAllocation() { failNextAllocation = true; }

  void storePage(TuplePageId pageId, const TuplePage &page) {
    pages.insert_or_assign(pageId, page);
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
  bool failNextAllocation{false};
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

TEST(TableTupleActionsTest, AllocationFailureDoesNotLeaveTailPagePinned) {
  InMemoryTupleStore store;
  InsertTupleIntoTableAction insert(store, store, store, store);
  const std::vector<char> firstRow(6000, 'a');
  const std::vector<char> secondRow(2500, 'b');
  const auto first =
      insert.execute(std::numeric_limits<TuplePageId>::max(), firstRow);

  store.failNextPageAllocation();
  EXPECT_THROW(insert.execute(first.firstPageId, secondRow),
               std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}

TEST(TableTupleActionsTest, ScanUnpinsPageWhenTupleMaterializationFails) {
  InMemoryTupleStore store;
  TuplePage corrupted;
  corrupted.insert({'x'});
  const uint32_t invalidOffset = 8191;
  std::memcpy(corrupted.data() + sizeof(uint32_t) * 3, &invalidOffset,
              sizeof(invalidOffset));
  store.storePage(99, corrupted);

  ScanTupleTableAction scan(store, store);
  EXPECT_THROW(scan.execute(99), std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}
