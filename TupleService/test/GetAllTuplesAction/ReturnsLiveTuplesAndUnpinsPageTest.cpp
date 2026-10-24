#include "../../actions/GetAllTuplesAction.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <vector>

namespace {
class FetchPort final : public TupleFetchPagePort {
public:
  TuplePage page;
  int calls = 0;
  bool shouldThrow = false;

  TuplePage fetchPage(TuplePageId) override {
    ++calls;
    if (shouldThrow) {
      throw std::runtime_error("fetch failed");
    }
    return page;
  }
};

class UnpinPort final : public TupleUnpinPagePort {
public:
  int calls = 0;
  TuplePageId lastPageId = 0;
  void unpinPage(TuplePageId pageId) override {
    ++calls;
    lastPageId = pageId;
  }
};
} // namespace

TEST(GetAllTuplesActionReturnsLiveTuplesAndUnpinsPageTest,
     ReturnsInsertedTuplesInSlotOrderAndSkipsDeletedSlots) {
  FetchPort fetch;
  fetch.page.insert({'a'});
  fetch.page.insert({'b', 'c'});
  fetch.page.insert({'d'});
  fetch.page.remove(1);
  UnpinPort unpin;
  GetAllTuplesAction action(fetch, unpin);

  EXPECT_EQ(action.execute(12),
            (std::vector<std::vector<char>>{{'a'}, {'d'}}));
  EXPECT_EQ(fetch.calls, 1);
  EXPECT_EQ(unpin.calls, 1);
  EXPECT_EQ(unpin.lastPageId, 12);
}

TEST(GetAllTuplesActionReturnsLiveTuplesAndUnpinsPageTest,
     EmptyPageReturnsEmptyCollectionAndUnpins) {
  FetchPort fetch;
  UnpinPort unpin;
  GetAllTuplesAction action(fetch, unpin);

  EXPECT_TRUE(action.execute(13).empty());
  EXPECT_EQ(unpin.calls, 1);
}

TEST(GetAllTuplesActionReturnsLiveTuplesAndUnpinsPageTest,
     FetchFailureDoesNotAttemptToUnpinUnfetchedPage) {
  FetchPort fetch;
  fetch.shouldThrow = true;
  UnpinPort unpin;
  GetAllTuplesAction action(fetch, unpin);

  EXPECT_THROW(action.execute(14), std::runtime_error);
  EXPECT_EQ(unpin.calls, 0);
}

TEST(GetAllTuplesActionReturnsLiveTuplesAndUnpinsPageTest,
     CorruptPageScanStillUnpinsPage) {
  FetchPort fetch;
  // A page with a corrupt slot directory must fail validation while scanning.
  auto *header = reinterpret_cast<uint32_t *>(fetch.page.data());
  header[0] = 0x80000001U;
  header[1] = 8;
  auto *slot = reinterpret_cast<uint32_t *>(fetch.page.data() + 8);
  slot[0] = 8190;
  slot[1] = 10;
  UnpinPort unpin;
  GetAllTuplesAction action(fetch, unpin);

  EXPECT_THROW(action.execute(15), std::runtime_error);
  EXPECT_EQ(unpin.calls, 1);
  EXPECT_EQ(unpin.lastPageId, 15);
}
