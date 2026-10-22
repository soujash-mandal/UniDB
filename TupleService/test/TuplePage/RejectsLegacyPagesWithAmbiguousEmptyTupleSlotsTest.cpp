#include "../../domain/TuplePage.h"
#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

// Scenario 1: Reject legacy pages whose empty tuple slot is ambiguous.
TEST(TuplePageTest, RejectsLegacyPagesWithAmbiguousEmptyTupleSlots) {
  std::vector<char> legacy(TUPLE_PAGE_SIZE, 0);
  const uint32_t slotCount = 1;
  const uint32_t freeSpaceOffset = TUPLE_PAGE_SIZE;
  const uint32_t emptyTupleOffset = TUPLE_PAGE_SIZE;
  const uint32_t tupleSize = 0;
  std::memcpy(legacy.data(), &slotCount, sizeof(slotCount));
  std::memcpy(legacy.data() + sizeof(uint32_t), &freeSpaceOffset,
              sizeof(freeSpaceOffset));
  std::memcpy(legacy.data() + sizeof(uint32_t) * 2, &emptyTupleOffset,
              sizeof(emptyTupleOffset));
  std::memcpy(legacy.data() + sizeof(uint32_t) * 3, &tupleSize,
              sizeof(tupleSize));

  TuplePage page;
  EXPECT_THROW(page.loadFromBytes(legacy.data()), std::runtime_error);
}
