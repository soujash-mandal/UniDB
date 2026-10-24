#include "GetTablePagesActionTestSupport.h"

// Scenario 6: Reject a corrupt entry count and still unpin the fetched page.
TEST(GetTablePagesActionTest, RejectsCorruptEntryCountAndUnpinsPage) {
  InMemorySpaceMapStore store;
  SpaceMapPage page;
  const uint32_t corruptEntryCount = UINT32_MAX;
  std::memcpy(page.data() + sizeof(SpaceMapPageId), &corruptEntryCount,
              sizeof(corruptEntryCount));
  store.seed(10, page);
  GetTablePagesAction action(store, store);

  EXPECT_THROW(action.execute(10), std::runtime_error);
  EXPECT_EQ(store.unpinnedPages, (std::vector<SpaceMapPageId>{10}));
  EXPECT_FALSE(store.hasPinnedPages());
}
