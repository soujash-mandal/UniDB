#pragma once

#include "../domain/SpaceMapPage.h"
#include "../port/FetchPagePort.h"
#include "../port/UnpinPagePort.h"

#include <vector>

class GetTablePagesAction {
public:
  GetTablePagesAction(FetchPagePort &fetchPagePort,
                      UnpinPagePort &unpinPagePort);

  // The root is the table's first SpaceMap page, stored in its catalog entry.
  // Returns tuple page IDs in SpaceMap traversal order.
  std::vector<TuplePageId> execute(SpaceMapPageId rootSpaceMapPageId);

private:
  FetchPagePort &fetchPagePort;
  UnpinPagePort &unpinPagePort;
};
