#include "GetTableDefinitionActionTestSupport.h"

#include <gtest/gtest.h>

// Scenario: the requested table name is resolved using the catalog root from
// metadata.
TEST(GetTableDefinitionActionTest, ReturnsDefinitionForRequestedTable) {
  GetTableDefinitionFakeRootPort rootPort;
  GetTableDefinitionFakeCatalogPort tablePort;
  RdbService::GetTableDefinitionAction action(rootPort, tablePort);

  const RdbTableDefinition result = action.execute(3, "events");

  EXPECT_EQ(rootPort.calls, 1U);
  EXPECT_EQ(rootPort.receivedMetadataPageId, 3U);
  EXPECT_EQ(tablePort.calls, 1U);
  EXPECT_EQ(tablePort.receivedCatalogRootPageId, 17U);
  EXPECT_EQ(tablePort.receivedTableName, "events");
  EXPECT_EQ(result.tableId, 42U);
  EXPECT_EQ(result.name, "events");
  EXPECT_EQ(result.firstSpaceMapPageId, 91U);
  ASSERT_EQ(result.columns.size(), 2U);
  EXPECT_EQ(result.columns[0].name, "event_id");
  EXPECT_EQ(result.columns[0].type, RdbDataType::INT);
  EXPECT_EQ(result.columns[1].name, "description");
  EXPECT_EQ(result.columns[1].type, RdbDataType::VARCHAR);
  EXPECT_EQ(result.columns[1].size, 120U);
  EXPECT_TRUE(result.columns[1].nullable);
}

// Scenario: an empty table name is rejected before metadata or catalog
// dependencies run.
TEST(GetTableDefinitionActionTest, RejectsEmptyNameBeforeCallingDependencies) {
  GetTableDefinitionFakeRootPort rootPort;
  GetTableDefinitionFakeCatalogPort tablePort;
  RdbService::GetTableDefinitionAction action(rootPort, tablePort);

  EXPECT_THROW(action.execute(3, ""), std::invalid_argument);

  EXPECT_EQ(rootPort.calls, 0U);
  EXPECT_EQ(tablePort.calls, 0U);
}

// Scenario: catalog lookup failures propagate to the caller without being
// hidden.
TEST(GetTableDefinitionActionTest, PropagatesCatalogLookupFailure) {
  GetTableDefinitionFakeRootPort rootPort;
  GetTableDefinitionFakeCatalogPort tablePort;
  tablePort.shouldThrow = true;
  RdbService::GetTableDefinitionAction action(rootPort, tablePort);

  EXPECT_THROW(action.execute(3, "events"), std::runtime_error);

  EXPECT_EQ(rootPort.calls, 1U);
  EXPECT_EQ(tablePort.calls, 1U);
}
