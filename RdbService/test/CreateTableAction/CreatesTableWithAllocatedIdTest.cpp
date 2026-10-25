#include "CreateTableActionTestSupport.h"

#include <gtest/gtest.h>

// Scenario: valid table creation uses the catalog root and allocated table ID.
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
  ASSERT_EQ(catalogPort.receivedTable.columns.size(), 2U);
  EXPECT_EQ(catalogPort.receivedTable.columns[0].name, "event_id");
  EXPECT_EQ(catalogPort.receivedTable.columns[0].type, RdbDataType::INT);
  EXPECT_EQ(catalogPort.receivedTable.columns[0].size, 4U);
  EXPECT_FALSE(catalogPort.receivedTable.columns[0].nullable);
  EXPECT_EQ(catalogPort.receivedTable.columns[1].name, "description");
  EXPECT_EQ(catalogPort.receivedTable.columns[1].type, RdbDataType::VARCHAR);
  EXPECT_EQ(catalogPort.receivedTable.columns[1].size, 120U);
  EXPECT_TRUE(catalogPort.receivedTable.columns[1].nullable);
}
