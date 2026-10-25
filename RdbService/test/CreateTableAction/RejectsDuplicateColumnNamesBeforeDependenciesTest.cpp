#include "CreateTableActionTestSupport.h"

#include <gtest/gtest.h>

// Scenario: duplicate column names are rejected before any dependency is called.
TEST(CreateTableActionTest,
     RejectsDuplicateColumnNamesBeforeCallingDependencies) {
  FakeGetCatalogRootPageIdPort rootPort;
  FakeAllocateNextTableIdPort tableIdPort;
  FakeCreateCatalogTablePort catalogPort;
  RdbService::CreateTableAction action(rootPort, tableIdPort, catalogPort);
  RdbTableDefinition table = validTableDefinition();
  table.columns.push_back(RdbColumn{"event_id", RdbDataType::BIGINT, 8, false});

  EXPECT_THROW(action.execute(3, table), std::invalid_argument);
  EXPECT_EQ(rootPort.calls, 0U);
  EXPECT_EQ(tableIdPort.calls, 0U);
  EXPECT_EQ(catalogPort.calls, 0U);
}
