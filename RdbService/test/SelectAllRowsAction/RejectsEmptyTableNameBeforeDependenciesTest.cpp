#include "SelectAllRowsActionTestSupport.h"

#include <gtest/gtest.h>

TEST(SelectAllRowsActionTest, RejectsEmptyTableNameBeforeDependencies) {
  // Covers rejecting invalid input before any dependency is invoked.
  FakeSelectRootPagePort rootPort;
  FakeSelectTableLookupPort tablePort;
  FakeSelectTablePagesPort pagesPort;
  FakeSelectAllTuplesPort tuplesPort;
  RdbService::SelectAllRowsAction action(rootPort, tablePort, pagesPort,
                                         tuplesPort);

  EXPECT_THROW(action.execute(3, ""), std::invalid_argument);
  EXPECT_EQ(rootPort.calls, 0U);
  EXPECT_EQ(tablePort.calls, 0U);
  EXPECT_EQ(pagesPort.calls, 0U);
  EXPECT_EQ(tuplesPort.calls, 0U);
}

