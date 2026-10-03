#pragma once

#include "../port/FetchPagePort.h"
#include "../port/NewPagePort.h"
#include "../port/UnpinPagePort.h"
#include "../port/WritePagePort.h"

class FindPageWithSpaceAction {
public:
  FindPageWithSpaceAction(FetchPagePort &fetchPagePort,
                          UnpinPagePort &unpinPagePort,
                          WritePagePort &writePagePort,
                          NewPagePort &newPagePort);

  TuplePageId execute(FSMPageId rootFsmPageId, uint32_t requiredSpace);

private:
  FetchPagePort &fetchPagePort;
  UnpinPagePort &unpinPagePort;
  WritePagePort &writePagePort;
  NewPagePort &newPagePort;
};
