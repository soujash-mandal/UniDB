#include "../../actions/GetTableDefinitionAction.h"
#include "../RdbActionTestSupport.h"

#include <gtest/gtest.h>

TEST(GetTableDefinitionActionTest, ReturnsTableDefinitionFromCatalogPort) {
  FakeRdbMetadataPort metadata;
  FakeCatalogTablePort catalog;
  catalog.storedTable.tableId = 7;
  catalog.storedTable.name = "users";
  catalog.storedTable.columns = {
      {"id", RdbDataType::INT, 4, false},
      {"name", RdbDataType::VARCHAR, 64, true},
  };
  GetTableDefinitionAction action(metadata, catalog);

  const auto result = action.execute("users");

  EXPECT_EQ(result.tableId, 7U);
  EXPECT_EQ(result.name, "users");
  EXPECT_EQ(result.columns.size(), 2U);
  EXPECT_EQ(catalog.lastRootPageId, 10U);
  EXPECT_EQ(catalog.lastLookupName, "users");
}
