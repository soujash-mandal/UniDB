#include "../../actions/CalculateRequiredSpaceAction.h"
#include <gtest/gtest.h>

#include <stdexcept>
#include <vector>

// Scenario 2: Reject tuple data that cannot fit on an otherwise empty page.
TEST(CalculateRequiredSpaceActionTest, RejectsTupleTooLargeForEmptyPage) {
  CalculateRequiredSpaceAction action;

  EXPECT_THROW(action.execute(std::vector<char>(8177, 'x')), std::length_error);
}
