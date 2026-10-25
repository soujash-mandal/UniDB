#include "../../adapter/CalculateRequiredSpaceAdapter.h"
#include "../../../TupleService/actions/CalculateRequiredSpaceAction.h"

#include <gtest/gtest.h>

TEST(CalculateRequiredSpaceAdapterTest,
     DelegatesRequiredSpaceCalculationToTupleServiceAction) {
  CalculateRequiredSpaceAction action;
  RdbService::CalculateRequiredSpaceAdapter adapter(action);
  const std::vector<char> tupleData{'a', 'b', 'c'};

  EXPECT_EQ(adapter.calculateRequiredSpace(tupleData), 3U + sizeof(uint32_t) * 2);
}
