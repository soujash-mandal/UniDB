#pragma once

#include "CreateTupleAction.h"

struct CreateTupleWithFreeSpaceResult {
  TupleSlotId slotId;
  uint32_t remainingFreeSpace;
};

class CreateTupleWithFreeSpaceAction {
public:
  CreateTupleWithFreeSpaceAction(TupleFetchPagePort &fetchPagePort,
                                 TupleWritePagePort &writePagePort,
                                 TupleUnpinPagePort &unpinPagePort);
  CreateTupleWithFreeSpaceResult execute(TuplePageId pageId,
                                         std::vector<char> tupleData);

private:
  TupleFetchPagePort &fetchPagePort;
  TupleWritePagePort &writePagePort;
  TupleUnpinPagePort &unpinPagePort;
};
