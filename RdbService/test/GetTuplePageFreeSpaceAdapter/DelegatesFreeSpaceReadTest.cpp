#include "../../adapter/GetTuplePageFreeSpaceAdapter.h"
#include "../../../TupleService/actions/GetTuplePageFreeSpaceAction.h"

#include <gtest/gtest.h>

namespace {
class Fetch final : public TupleFetchPagePort {
public:
  TuplePage fetchPage(TuplePageId pageId) override {
    requestedPageId = pageId;
    return page;
  }
  TuplePage page;
  TuplePageId requestedPageId = 0;
};

class Unpin final : public TupleUnpinPagePort {
public:
  void unpinPage(TuplePageId pageId) override {
    ++calls;
    requestedPageId = pageId;
  }
  int calls = 0;
  TuplePageId requestedPageId = 0;
};
} // namespace

TEST(GetTuplePageFreeSpaceAdapterTest, DelegatesFreeSpaceRead) {
  // Scenario 1: Forwards the tuple page ID and returns its actual free space.
  Fetch fetch;
  fetch.page.insert({'a', 'b'});
  Unpin unpin;
  GetTuplePageFreeSpaceAction action(fetch, unpin);
  RdbService::GetTuplePageFreeSpaceAdapter adapter(action);

  EXPECT_EQ(adapter.getFreeSpace(23), 8174);
  EXPECT_EQ(fetch.requestedPageId, 23);
  EXPECT_EQ(unpin.calls, 1);
  EXPECT_EQ(unpin.requestedPageId, 23);
}
