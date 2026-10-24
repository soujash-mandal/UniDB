#include "../../actions/RdbCreateTableAction.h"
#include "../RdbActionTestSupport.h"

#include <gtest/gtest.h>

#include <stdexcept>

TEST(RdbCreateTableActionTest, RejectsDuplicateColumnsBeforeAllocatingTableId) {
  FakeRdbMetadataPort metadata;
  FakeCatalogTablePort catalog;
  RdbCreateTableAction action(metadata, catalog);

  EXPECT_THROW(action.execute("users", {{"id", RdbDataType::INT, 4, false},
                                        {"id", RdbDataType::BIGINT, 8, false}}),
               std::invalid_argument);
  EXPECT_EQ(metadata.rootLookups, 0U);
  EXPECT_EQ(metadata.tableIdAllocations, 0U);
  EXPECT_EQ(catalog.createCalls, 0U);
}
