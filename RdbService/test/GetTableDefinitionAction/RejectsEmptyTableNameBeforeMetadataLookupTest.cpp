#include "../../actions/GetTableDefinitionAction.h"
#include "../RdbActionTestSupport.h"

#include <gtest/gtest.h>

#include <stdexcept>

TEST(GetTableDefinitionActionTest,
     RejectsEmptyTableNameBeforeMetadataLookup) {
  FakeRdbMetadataPort metadata;
  FakeCatalogTablePort catalog;
  GetTableDefinitionAction action(metadata, catalog);

  EXPECT_THROW(action.execute(""), std::invalid_argument);
  EXPECT_EQ(metadata.rootLookups, 0U);
  EXPECT_EQ(catalog.lookupCalls, 0U);
}
