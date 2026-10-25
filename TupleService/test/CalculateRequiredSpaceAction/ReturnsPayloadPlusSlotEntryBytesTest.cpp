#include "../../actions/CalculateRequiredSpaceAction.h"
#include <gtest/gtest.h>

#include <vector>

// Scenario 1: Required space includes tuple payload bytes and one slot entry.
TEST(CalculateRequiredSpaceActionTest, ReturnsPayloadPlusSlotEntryBytes) {
  CalculateRequiredSpaceAction action;

  EXPECT_EQ(action.execute(std::vector<char>(42, 'x')), 50U);
}
