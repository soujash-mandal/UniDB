#include "../../domain/TuplePage.h"
#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <stdexcept>

// Scenario 1: Reject corrupt tuple offsets during a page scan.
TEST(TuplePageTest, RejectsCorruptSlotBoundsDuringScan) {
  TuplePage page;
  page.insert({'a', 'b'});
  // V1 layout: 12-byte header followed by 8-byte slot entries.
  const uint32_t invalidOffset = 8191;
  std::memcpy(page.data() + sizeof(uint32_t) * 2, &invalidOffset,
              sizeof(invalidOffset));
  EXPECT_THROW(page.getAllTuples(), std::runtime_error);
}
