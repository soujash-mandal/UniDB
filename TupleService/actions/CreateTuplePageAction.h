#pragma once

#include "../port/NewPagePort.h"
#include "../port/UnpinPagePort.h"
#include "../port/WritePagePort.h"

struct CreateTuplePageActionResult {
  TuplePageId pageId;
  uint32_t freeSpace;
};

class CreateTuplePageAction {
public:
  explicit CreateTuplePageAction(TupleNewPagePort &newPagePort,
                                 TupleWritePagePort &writePagePort,
                                 TupleUnpinPagePort &unpinPagePort);
  CreateTuplePageActionResult execute();
private:
  TupleNewPagePort &newPagePort;
  TupleWritePagePort &writePagePort;
  TupleUnpinPagePort &unpinPagePort;
};
