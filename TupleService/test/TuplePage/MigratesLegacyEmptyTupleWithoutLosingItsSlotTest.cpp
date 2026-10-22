#include "../../domain/TuplePage.h"
#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <vector>

// Scenario 1: Preserve legacy empty and non-empty tuples during migration.
TEST(TuplePageTest, MigratesLegacyEmptyTupleWithoutLosingItsSlot) {
  std::vector<char> legacy(TUPLE_PAGE_SIZE, 0);
  const uint32_t slotCount = 2;
  const uint32_t freeSpaceOffset = TUPLE_PAGE_SIZE - 1;
  const uint32_t emptyTupleOffset = TUPLE_PAGE_SIZE;
  const uint32_t emptyTupleSize = 0;
  const uint32_t tupleOffset = TUPLE_PAGE_SIZE - 1;
  const uint32_t tupleSize = 1;
  std::memcpy(legacy.data(), &slotCount, sizeof(slotCount));
  std::memcpy(legacy.data() + sizeof(uint32_t), &freeSpaceOffset,
              sizeof(freeSpaceOffset));
  std::memcpy(legacy.data() + sizeof(uint32_t) * 2, &emptyTupleOffset,
              sizeof(emptyTupleOffset));
  std::memcpy(legacy.data() + sizeof(uint32_t) * 3, &emptyTupleSize,
              sizeof(emptyTupleSize));
  std::memcpy(legacy.data() + sizeof(uint32_t) * 4, &tupleOffset,
              sizeof(tupleOffset));
  std::memcpy(legacy.data() + sizeof(uint32_t) * 5, &tupleSize,
              sizeof(tupleSize));
  legacy[tupleOffset] = 'z';

  TuplePage page(legacy.data());

  EXPECT_TRUE(page.get(0).empty());
  EXPECT_EQ(page.get(1), (std::vector<char>{'z'}));
  const auto tuples = page.getAllTuples();
  ASSERT_EQ(tuples.size(), 2U);
  EXPECT_TRUE(tuples[0].empty());
  EXPECT_EQ(tuples[1], (std::vector<char>{'z'}));
}
