#include "../../actions/RdbCreateTableAction.h"
#include "../RdbActionTestSupport.h"

#include <gtest/gtest.h>

TEST(RdbCreateTableActionTest, CreatesTableUsingMetadataAndCatalogPorts) {
  FakeRdbMetadataPort metadata;
  FakeCatalogTablePort catalog;
  RdbCreateTableAction action(metadata, catalog);

  const std::vector<RdbColumn> columns = {
      {"id", RdbDataType::INT, 4, false},
      {"name", RdbDataType::VARCHAR, 64, true},
  };
  const auto tableId = action.execute("users", columns);

  EXPECT_EQ(tableId, 1U);
  EXPECT_EQ(metadata.rootLookups, 1U);
  EXPECT_EQ(metadata.tableIdAllocations, 1U);
  EXPECT_EQ(catalog.lastRootPageId, 10U);
  EXPECT_EQ(catalog.lastTableId, 1U);
  EXPECT_EQ(catalog.lastCreatedTable.name, "users");
  EXPECT_EQ(catalog.lastCreatedTable.columns.size(), 2U);
}
