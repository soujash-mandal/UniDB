#pragma once

#include <vector>

#include "../domain/TuplePage.h"
#include "../port/FetchPagePort.h"
#include "../port/UnpinPagePort.h"

// Reads every live tuple on one tuple page. Table-level page discovery remains
// the responsibility of the caller (for example, RDBService).
class GetAllTuplesAction {
public:
  GetAllTuplesAction(TupleFetchPagePort &fetchPagePort,
                     TupleUnpinPagePort &unpinPagePort);

  std::vector<std::vector<char>> execute(TuplePageId pageId);

private:
  TupleFetchPagePort &fetchPagePort;
  TupleUnpinPagePort &unpinPagePort;
};
