#include "SelectAllRowsActionTestSupport.h"

#include <gtest/gtest.h>

TEST(SelectAllRowsActionTest, PropagatesPageEnumerationFailure) {
  // Covers surfacing SpaceMap failures instead of returning partial rows.
  FakeSelectRootPagePort rootPort;
  FakeSelectTableLookupPort tablePort;
  FakeSelectTablePagesPort pagesPort;
  FakeSelectAllTuplesPort tuplesPort;
  tablePort.table.firstSpaceMapPageId = 29;
  pagesPort.shouldThrow = true;
  RdbService::SelectAllRowsAction action(rootPort, tablePort, pagesPort,
                                         tuplesPort);

  EXPECT_THROW(action.execute(3, "events"), std::runtime_error);
  EXPECT_EQ(tuplesPort.calls, 0U);
}
