#include "CreateTableActionTestSupport.h"

#include <gtest/gtest.h>

// Scenario: an empty table name is rejected before any dependency is called.
TEST(CreateTableActionTest, RejectsEmptyTableNameBeforeCallingDependencies) {
  FakeGetCatalogRootPageIdPort rootPort;
  FakeAllocateNextTableIdPort tableIdPort;
  FakeCreateCatalogTablePort catalogPort;
  RdbService::CreateTableAction action(rootPort, tableIdPort, catalogPort);
  RdbTableDefinition table = validTableDefinition();
  table.name.clear();

  EXPECT_THROW(action.execute(3, table), std::invalid_argument);
  EXPECT_EQ(rootPort.calls, 0U);
  EXPECT_EQ(tableIdPort.calls, 0U);
  EXPECT_EQ(catalogPort.calls, 0U);
}
