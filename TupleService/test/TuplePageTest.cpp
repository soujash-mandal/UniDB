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
  std::memcpy(page.data() + sizeof(uint32_t) * 3, &invalidOffset,
              sizeof(invalidOffset));
  EXPECT_THROW(page.getAllTuples(), std::runtime_error);
}

TEST(TuplePageTest, RejectsCorruptSlotCountDuringScan) {
  TuplePage page;
  const uint32_t invalidSlotCount = 2000;
  std::memcpy(page.data(), &invalidSlotCount, sizeof(invalidSlotCount));
  EXPECT_THROW(page.getAllTuples(), std::runtime_error);
}
