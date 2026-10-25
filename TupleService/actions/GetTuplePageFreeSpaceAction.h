#pragma once

#include "../port/FetchPagePort.h"
#include "../port/UnpinPagePort.h"

#include <cstdint>

class GetTuplePageFreeSpaceAction {
public:
  GetTuplePageFreeSpaceAction(TupleFetchPagePort &fetchPagePort,
                              TupleUnpinPagePort &unpinPagePort);

  uint32_t execute(TuplePageId pageId);

private:
  TupleFetchPagePort &fetchPagePort;
  TupleUnpinPagePort &unpinPagePort;
};
