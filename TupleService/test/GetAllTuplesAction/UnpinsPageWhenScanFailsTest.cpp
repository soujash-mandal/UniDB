#include "GetAllTuplesActionTestSupport.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <stdexcept>

// Scenario 5: Release the fetched page even when tuple-page validation fails.
TEST(GetAllTuplesActionTest, UnpinsPageWhenScanFails) {
  GetAllTuplesFetchPort fetch;
  // A valid-format header whose slot count cannot fit before the free-space
  // boundary.
  const uint32_t corruptSlotCount = 0x80000001U;
  const uint32_t invalidFreeSpaceOffset = 8U;
  std::memcpy(fetch.page.data(), &corruptSlotCount, sizeof(corruptSlotCount));
  std::memcpy(fetch.page.data() + sizeof(uint32_t), &invalidFreeSpaceOffset,
              sizeof(invalidFreeSpaceOffset));
  GetAllTuplesUnpinPort unpin;
  GetAllTuplesAction action(fetch, unpin);

  EXPECT_THROW(action.execute(16), std::runtime_error);
  EXPECT_EQ(unpin.calls, 1);
  EXPECT_EQ(unpin.lastPageId, 16);
}
