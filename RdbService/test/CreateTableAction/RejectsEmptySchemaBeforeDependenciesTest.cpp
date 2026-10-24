#include "CreateTableActionTestSupport.h"

#include <gtest/gtest.h>

TEST(CreateTableActionTest, RejectsEmptySchemaBeforeCallingDependencies) {
  FakeGetCatalogRootPageIdPort rootPort;
  FakeAllocateNextTableIdPort tableIdPort;
  FakeCreateCatalogTablePort catalogPort;
  RdbService::CreateTableAction action(rootPort, tableIdPort, catalogPort);
  RdbTableDefinition table;
  table.name = "events";

  EXPECT_THROW(action.execute(3, table), std::invalid_argument);
  EXPECT_EQ(rootPort.calls, 0U);
  EXPECT_EQ(tableIdPort.calls, 0U);
  EXPECT_EQ(catalogPort.calls, 0U);
}
