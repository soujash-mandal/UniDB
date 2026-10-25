#include "GetTableDefinitionActionTestSupport.h"

#include <gtest/gtest.h>

TEST(GetTableDefinitionActionTest, ReturnsDefinitionForExistingTable) {
  // Covers forwarding the metadata root and returning the complete table schema.
  FakeDefinitionRootPagePort rootPort;
  FakeDefinitionTableLookupPort tablePort;
  tablePort.result = validDefinitionForLookup();
  RdbService::GetTableDefinitionAction action(rootPort, tablePort);

  const auto result = action.execute(3, "events");

  EXPECT_EQ(rootPort.calls, 1U);
  EXPECT_EQ(rootPort.receivedMetadataPageId, 3U);
  EXPECT_EQ(tablePort.calls, 1U);
  EXPECT_EQ(tablePort.receivedRootPageId, 17U);
  EXPECT_EQ(tablePort.receivedTableName, "events");
  EXPECT_EQ(result.tableId, 42U);
  EXPECT_EQ(result.name, "events");
  ASSERT_EQ(result.columns.size(), 2U);
  EXPECT_EQ(result.columns[0].name, "event_id");
  EXPECT_EQ(result.columns[0].type, RdbDataType::INT);
  EXPECT_EQ(result.columns[0].size, 4U);
  EXPECT_FALSE(result.columns[0].nullable);
  EXPECT_EQ(result.columns[1].name, "description");
  EXPECT_EQ(result.columns[1].type, RdbDataType::VARCHAR);
  EXPECT_EQ(result.columns[1].size, 120U);
  EXPECT_TRUE(result.columns[1].nullable);
}
