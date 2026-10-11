#pragma once

#include "../port/FetchPagePort.h"
#include "../port/UnpinPagePort.h"
#include <vector>

class ScanTupleTableAction {
public:
  ScanTupleTableAction(TupleFetchPagePort &fetchPagePort,
                       TupleUnpinPagePort &unpinPagePort);
  std::vector<std::vector<char>> execute(TuplePageId firstPageId);

private:
  TupleFetchPagePort &fetchPagePort;
  TupleUnpinPagePort &unpinPagePort;
};
