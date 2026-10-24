#include "GetAllTuplesActionTestSupport.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <stdexcept>

// Scenario 5: Preserve the scan error and release the fetched page even when
// unpinning also fails.
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
  unpin.failUnpin = true;
  GetAllTuplesAction action(fetch, unpin);

  try {
    action.execute(16);
    FAIL() << "execute should throw";
  } catch (const std::runtime_error &error) {
    EXPECT_STREQ(error.what(), "Tuple page slots overlap tuple data");
  }
  EXPECT_EQ(unpin.calls, 1);
  EXPECT_EQ(unpin.lastPageId, 16);
}