#include "../../../TupleService/actions/CreateTuplePageAction.h"
#include "../../../TupleService/actions/CreateTupleWithFreeSpaceAction.h"
#include "../../adapter/CreateTupleWithFreeSpaceAdapter.h"
#include "../StorageAdapterTestSupport.h"

#include <gtest/gtest.h>

TEST(CreateTupleWithFreeSpaceAdapterTest, ReturnsSlotAndRemainingSpace) {
  // Scenario 1: Maps the TupleService insert result to the RDB-owned result.
  FakeTupleStore store;
  CreateTuplePageAction createPage(store, store, store);
  const auto page = createPage.execute();
  CreateTupleWithFreeSpaceAction action(store, store, store);
  RdbService::CreateTupleWithFreeSpaceAdapter adapter(action);

  const auto result =
      adapter.createTupleWithFreeSpace(page.pageId, {'r', 'o', 'w'});

  EXPECT_EQ(result.slotId, 0U);
  EXPECT_EQ(result.remainingFreeSpace, 8173U);
}
