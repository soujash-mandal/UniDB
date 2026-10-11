#pragma once

#include "../port/FetchPagePort.h"
#include "../port/UnpinPagePort.h"

#include <vector>

class ScanTupleTableAction {
public:
  ScanTupleTableAction(FetchPagePort &fetchPagePort,
                       UnpinPagePort &unpinPagePort);

  std::vector<std::vector<char>> execute(TuplePageId firstPageId);

private:
  FetchPagePort &fetchPagePort;
  UnpinPagePort &unpinPagePort;
};
