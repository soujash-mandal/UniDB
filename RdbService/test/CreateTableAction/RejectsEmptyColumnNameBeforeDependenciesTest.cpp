#include "CreateTableActionTestSupport.h"

#include <gtest/gtest.h>

// Scenario: an empty column name is rejected before any dependency is called.
TEST(CreateTableActionTest, RejectsEmptyColumnNameBeforeCallingDependencies) {
  FakeGetCatalogRootPageIdPort rootPort;
  FakeAllocateNextTableIdPort tableIdPort;
  FakeCreateCatalogTablePort catalogPort;
  RdbService::CreateTableAction action(rootPort, tableIdPort, catalogPort);
  RdbTableDefinition table = validTableDefinition();
  table.columns[0].name.clear();

  EXPECT_THROW(action.execute(3, table), std::invalid_argument);
  EXPECT_EQ(rootPort.calls, 0U);
  EXPECT_EQ(tableIdPort.calls, 0U);
  EXPECT_EQ(catalogPort.calls, 0U);
}
