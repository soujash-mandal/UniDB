#include "CreateTableActionTestSupport.h"

#include <gtest/gtest.h>

// Scenario: a zero-sized VARCHAR column is rejected before dependencies run.
TEST(CreateTableActionTest, RejectsZeroSizedVarcharBeforeCallingDependencies) {
  FakeGetCatalogRootPageIdPort rootPort;
  FakeAllocateNextTableIdPort tableIdPort;
  FakeCreateCatalogTablePort catalogPort;
  RdbService::CreateTableAction action(rootPort, tableIdPort, catalogPort);
  RdbTableDefinition table = validTableDefinition();
  table.columns[1].size = 0;

  EXPECT_THROW(action.execute(3, table), std::invalid_argument);
  EXPECT_EQ(rootPort.calls, 0U);
  EXPECT_EQ(tableIdPort.calls, 0U);
  EXPECT_EQ(catalogPort.calls, 0U);
}
