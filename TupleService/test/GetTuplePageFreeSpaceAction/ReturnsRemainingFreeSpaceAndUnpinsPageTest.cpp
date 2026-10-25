#include "../../actions/GetTuplePageFreeSpaceAction.h"

#include <gtest/gtest.h>

namespace {
class Fetch final : public TupleFetchPagePort {
public:
  TuplePage page;
  TuplePage fetchPage(TuplePageId pageId) override {
    requestedPageId = pageId;
    return page;
  }
  TuplePageId requestedPageId = 0;
};

class Unpin final : public TupleUnpinPagePort {
public:
  void unpinPage(TuplePageId pageId) override {
    ++calls;
    unpinnedPageId = pageId;
  }
  int calls = 0;
  TuplePageId unpinnedPageId = 0;
};
} // namespace

TEST(GetTuplePageFreeSpaceActionTest, ReturnsRemainingFreeSpaceAndUnpinsPage) {
  // Scenario 1: Reports actual remaining free space from the fetched tuple
  // page.
  Fetch fetch;
  fetch.page.insert({'a', 'b'});
  Unpin unpin;
  GetTuplePageFreeSpaceAction action(fetch, unpin);

  const uint32_t freeSpace = action.execute(23);

  EXPECT_EQ(freeSpace, 8174);
  EXPECT_EQ(fetch.requestedPageId, 23);
  EXPECT_EQ(unpin.calls, 1);
  EXPECT_EQ(unpin.unpinnedPageId, 23);
}
