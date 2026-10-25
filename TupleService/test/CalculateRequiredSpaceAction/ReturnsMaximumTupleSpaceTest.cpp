#include "../../actions/CalculateRequiredSpaceAction.h"
#include <gtest/gtest.h>

#include <vector>

// Scenario 3: The largest tuple that fits consumes all initial free space.
TEST(CalculateRequiredSpaceActionTest, ReturnsMaximumTupleSpace) {
  CalculateRequiredSpaceAction action;

  EXPECT_EQ(action.execute(std::vector<char>(8176, 'x')), 8184U);
}
