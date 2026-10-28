#pragma once

#include <cstdint>
#include <vector>

#include "../domain/TuplePage.h"
#include "../port/FetchPagePort.h"
#include "../port/UnpinPagePort.h"
#include "../port/WritePagePort.h"

struct CreateTupleActionResult {
  TupleSlotId slotId;
  uint32_t freeSpace;
};

class CreateTupleAction {
public:
  CreateTupleAction(TupleFetchPagePort &fetchPagePort,
                    TupleWritePagePort &writePagePort,
                    TupleUnpinPagePort &unpinPagePort);
  CreateTupleActionResult execute(TuplePageId pageId,
                                  std::vector<char> tupleData);

private:
  TupleFetchPagePort &fetchPagePort;
  TupleWritePagePort &writePagePort;
  TupleUnpinPagePort &unpinPagePort;
};
