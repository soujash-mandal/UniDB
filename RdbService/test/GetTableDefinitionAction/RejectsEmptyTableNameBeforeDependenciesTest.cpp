#include "GetTableDefinitionActionTestSupport.h"

#include <gtest/gtest.h>

TEST(GetTableDefinitionActionTest, RejectsEmptyTableNameBeforeDependencies) {
  // Covers invalid empty names without calling either dependency.
  FakeDefinitionRootPagePort rootPort;
  FakeDefinitionTableLookupPort tablePort;
  RdbService::GetTableDefinitionAction action(rootPort, tablePort);

  EXPECT_THROW(action.execute(3, ""), std::invalid_argument);
  EXPECT_EQ(rootPort.calls, 0U);
  EXPECT_EQ(tablePort.calls, 0U);
}
