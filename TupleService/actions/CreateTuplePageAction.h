#pragma once

#include "../port/NewPagePort.h"
#include "../port/UnpinPagePort.h"
#include "../port/WritePagePort.h"

struct CreateTuplePageResult {
  TuplePageId pageId;
  uint32_t freeSpace;
};

class CreateTuplePageAction {
public:
  explicit CreateTuplePageAction(NewPagePort &newPagePort,
                                 WritePagePort &writePagePort,
                                 UnpinPagePort &unpinPagePort);
  CreateTuplePageResult execute();

private:
  NewPagePort &newPagePort;
  WritePagePort &writePagePort;
  UnpinPagePort &unpinPagePort;
};
