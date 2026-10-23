#include "../../domain/TuplePage.h"
#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <stdexcept>

// Scenario 1: Reject corrupt slot counts during a page scan.
TEST(TuplePageTest, RejectsCorruptSlotCountDuringScan) {
  TuplePage page;
  const uint32_t invalidSlotCount = TUPLE_PAGE_FORMAT_FLAG | 2000U;
  std::memcpy(page.data(), &invalidSlotCount, sizeof(invalidSlotCount));
  EXPECT_THROW(page.getAllTuples(), std::runtime_error);
}
