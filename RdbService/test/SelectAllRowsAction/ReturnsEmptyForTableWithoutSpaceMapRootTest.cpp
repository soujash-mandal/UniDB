#include "SelectAllRowsActionTestSupport.h"

#include <gtest/gtest.h>

#include <limits>

TEST(SelectAllRowsActionTest, ReturnsEmptyForTableWithoutSpaceMapRoot) {
  // Covers a newly created table with no tuple-page inventory yet.
  FakeSelectRootPagePort rootPort;
  FakeSelectTableLookupPort tablePort;
  FakeSelectTablePagesPort pagesPort;
  FakeSelectAllTuplesPort tuplesPort;
  tablePort.table.firstSpaceMapPageId = std::numeric_limits<uint32_t>::max();
  RdbService::SelectAllRowsAction action(rootPort, tablePort, pagesPort,
                                         tuplesPort);

  const auto rows = action.execute(3, "events");

  EXPECT_TRUE(rows.empty());
  EXPECT_EQ(pagesPort.calls, 0U);
  EXPECT_EQ(tuplesPort.calls, 0U);
}

