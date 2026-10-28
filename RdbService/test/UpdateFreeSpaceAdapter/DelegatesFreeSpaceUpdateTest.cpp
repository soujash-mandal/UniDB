#include "../../../SpaceMapService/actions/UpdateFreeSpaceAction.h"
#include "../../adapter/UpdateFreeSpaceAdapter.h"
#include <gtest/gtest.h>

namespace {
class Fetch final : public FetchPagePort {
public:
  SpaceMapPage fetchPage(SpaceMapPageId) override {
    SpaceMapPage page;
    page.insert(34, 0);
    return page;
  }
};
class Write final : public SpaceMapWritePagePort {
public:
  void writePage(SpaceMapPageId, SpaceMapPage) override { ++calls; }
  int calls = 0;
};
class Unpin final : public UnpinPagePort {
public:
  void unpinPage(SpaceMapPageId) override { ++calls; }
  int calls = 0;
};
} // namespace

TEST(UpdateFreeSpaceAdapterTest, DelegatesFreeSpaceUpdate) {
  // Scenario 1: forwards page IDs and free-space update to SpaceMapService.
  Fetch fetch;
  Write write;
  Unpin unpin;
  UpdateFreeSpaceAction action(fetch, write, unpin);
  RdbService::UpdateFreeSpaceAdapter adapter(action);
  adapter.updateFreeSpace(12, 34, 4096);
  EXPECT_EQ(write.calls, 1);
  EXPECT_EQ(unpin.calls, 1);
}
