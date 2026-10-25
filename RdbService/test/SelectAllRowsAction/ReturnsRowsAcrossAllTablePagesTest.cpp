#include "SelectAllRowsActionTestSupport.h"

#include <gtest/gtest.h>

TEST(SelectAllRowsActionTest, ReturnsRowsAcrossAllTablePages) {
  // Covers preserving page traversal order while combining live rows.
  FakeSelectRootPagePort rootPort;
  FakeSelectTableLookupPort tablePort;
  FakeSelectTablePagesPort pagesPort;
  FakeSelectAllTuplesPort tuplesPort;
  tablePort.table.name = "events";
  tablePort.table.firstSpaceMapPageId = 29;
  pagesPort.pageIds = {101, 202};
  setRows(tuplesPort, 101, {{'a'}, {'b'}});
  setRows(tuplesPort, 202, {{'c'}});
  RdbService::SelectAllRowsAction action(rootPort, tablePort, pagesPort,
                                         tuplesPort);

  const auto rows = action.execute(3, "events");

  EXPECT_EQ(rootPort.metadataPageId, 3U);
  EXPECT_EQ(tablePort.catalogRootPageId, 17U);
  EXPECT_EQ(tablePort.tableName, "events");
  EXPECT_EQ(pagesPort.rootSpaceMapPageId, 29U);
  ASSERT_EQ(rows.size(), 3U);
  EXPECT_EQ(rows[0], (std::vector<char>{'a'}));
  EXPECT_EQ(rows[1], (std::vector<char>{'b'}));
  EXPECT_EQ(rows[2], (std::vector<char>{'c'}));
  EXPECT_EQ(tuplesPort.requestedPageIds, (std::vector<uint32_t>{101, 202}));
}

