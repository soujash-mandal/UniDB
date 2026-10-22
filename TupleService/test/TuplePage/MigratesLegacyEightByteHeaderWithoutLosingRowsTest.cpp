#include "../../domain/TuplePage.h"
#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <vector>

// Scenario 1: Migrate a legacy page without losing its live tuple.
TEST(TuplePageTest, MigratesLegacyEightByteHeaderWithoutLosingRows) {
  std::vector<char> legacy(TUPLE_PAGE_SIZE, 0);
  const uint32_t slotCount = 1;
  const uint32_t freeSpaceOffset = TUPLE_PAGE_SIZE - 1;
  const uint32_t tupleOffset = TUPLE_PAGE_SIZE - 1;
  const uint32_t tupleSize = 1;
  std::memcpy(legacy.data(), &slotCount, sizeof(slotCount));
  std::memcpy(legacy.data() + sizeof(uint32_t), &freeSpaceOffset,
              sizeof(freeSpaceOffset));
  std::memcpy(legacy.data() + sizeof(uint32_t) * 2, &tupleOffset,
              sizeof(tupleOffset));
  std::memcpy(legacy.data() + sizeof(uint32_t) * 3, &tupleSize,
              sizeof(tupleSize));
  legacy[tupleOffset] = 'z';

  TuplePage page;
  page.loadFromBytes(legacy.data());

  EXPECT_EQ(page.get(0), (std::vector<char>{'z'}));
  EXPECT_EQ(page.getAllTuples(), (std::vector<std::vector<char>>{{'z'}}));
}
