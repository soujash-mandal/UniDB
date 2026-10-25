#pragma once

#include "../domain/TuplePage.h"
#include "../port/FetchPagePort.h"
#include "../port/UnpinPagePort.h"
#include "../port/WritePagePort.h"

#include <cstdint>
#include <vector>

struct CreateTupleWithFreeSpaceActionResult {
  TupleSlotId slotId;
  uint32_t remainingFreeSpace;
};

class CreateTupleWithFreeSpaceAction {
public:
  CreateTupleWithFreeSpaceAction(TupleFetchPagePort &fetchPagePort,
                                 TupleWritePagePort &writePagePort,
                                 TupleUnpinPagePort &unpinPagePort);

  CreateTupleWithFreeSpaceActionResult execute(TuplePageId pageId,
                                                std::vector<char> tupleData);

private:
  TupleFetchPagePort &fetchPagePort;
  TupleWritePagePort &writePagePort;
  TupleUnpinPagePort &unpinPagePort;
};
