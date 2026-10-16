#include "../domain/TuplePage.h"
#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <stdexcept>

TEST(TuplePageTest, MaintainsPageChainAndSkipsDeletedSlots) {
  TuplePage page;
  const auto first = page.insert({'a', 'b'});
  page.insert({'c'});
  page.remove(first);
  EXPECT_EQ(page.getAllTuples().size(), 1U);
  EXPECT_EQ(page.getAllTuples().front(), (std::vector<char>{'c'}));
  page.setNextPageId(17);
  EXPECT_EQ(page.getNextPageId(), 17U);
}

TEST(TuplePageTest, RejectsCorruptSlotBoundsDuringScan) {
  TuplePage page;
  page.insert({'a', 'b'});
  // V1 layout: 12-byte header followed by 8-byte slot entries.
  const uint32_t invalidOffset = 8191;
  std::memcpy(page.data() + sizeof(uint32_t) * 2, &invalidOffset,
              sizeof(invalidOffset));
  EXPECT_THROW(page.getAllTuples(), std::runtime_error);
}

TEST(TuplePageTest, RejectsCorruptSlotCountDuringScan) {
  TuplePage page;
  const uint32_t invalidSlotCount = TUPLE_PAGE_FORMAT_FLAG | 2000U;
  std::memcpy(page.data(), &invalidSlotCount, sizeof(invalidSlotCount));
  EXPECT_THROW(page.getAllTuples(), std::runtime_error);
}


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
  EXPECT_EQ(page.getNextPageId(), std::numeric_limits<TuplePageId>::max());
}
