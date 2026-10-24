#include "CreateTableActionTestSupport.h"

#include <gtest/gtest.h>

TEST(CreateTableActionTest, CreatesTableWithAllocatedIdAndCatalogRoot) {
  FakeGetCatalogRootPageIdPort rootPort;
  FakeAllocateNextTableIdPort tableIdPort;
  FakeCreateCatalogTablePort catalogPort;
  RdbService::CreateTableAction action(rootPort, tableIdPort, catalogPort);

  const uint32_t result = action.execute(3, validTableDefinition());

  EXPECT_EQ(result, 42U);
  EXPECT_EQ(rootPort.calls, 1U);
  EXPECT_EQ(rootPort.receivedMetadataPageId, 3U);
  EXPECT_EQ(tableIdPort.calls, 1U);
  EXPECT_EQ(tableIdPort.receivedMetadataPageId, 3U);
  EXPECT_EQ(catalogPort.calls, 1U);
  EXPECT_EQ(catalogPort.receivedRootPageId, 17U);
  EXPECT_EQ(catalogPort.receivedTableId, 42U);
  EXPECT_EQ(catalogPort.receivedTable.tableId, 42U);
  EXPECT_EQ(catalogPort.receivedTable.name, "events");
}
