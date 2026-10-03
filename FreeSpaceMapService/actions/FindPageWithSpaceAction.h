#pragma once

#include "../port/CreateTuplePagePort.h"
#include "../port/FetchPagePort.h"
#include "../port/NewPagePort.h"
#include "../port/UnpinPagePort.h"
#include "../port/WritePagePort.h"

class FindPageWithSpaceAction {
public:
  FindPageWithSpaceAction(FetchPagePort &fetchPagePort,
                          UnpinPagePort &unpinPagePort,
                          WritePagePort &writePagePort,
                          NewPagePort &newPagePort,
                          CreateTuplePagePort &createTuplePagePort,
                          uint32_t newTuplePageFreeSpace);

  TuplePageId execute(FSMPageId rootFsmPageId, uint32_t requiredSpace);

private:
  FetchPagePort &fetchPagePort;
  UnpinPagePort &unpinPagePort;
  WritePagePort &writePagePort;
  NewPagePort &newPagePort;
  CreateTuplePagePort &createTuplePagePort;
  uint32_t newTuplePageFreeSpace;
};
