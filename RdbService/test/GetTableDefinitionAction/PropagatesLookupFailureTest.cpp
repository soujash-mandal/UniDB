#include "GetTableDefinitionActionTestSupport.h"

#include <gtest/gtest.h>

TEST(GetTableDefinitionActionTest, PropagatesLookupFailureAfterResolvingRoot) {
  // Covers preserving the catalog lookup error after resolving the root page.
  FakeDefinitionRootPagePort rootPort;
  FakeDefinitionTableLookupPort tablePort;
  tablePort.shouldThrow = true;
  RdbService::GetTableDefinitionAction action(rootPort, tablePort);

  EXPECT_THROW(action.execute(3, "events"), std::runtime_error);
  EXPECT_EQ(rootPort.calls, 1U);
  EXPECT_EQ(tablePort.calls, 1U);
}
