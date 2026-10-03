#pragma once

#include "../port/FetchPagePort.h"
#include "../port/UnpinPagePort.h"

class FindPageWithSpaceAction {
public:
  FindPageWithSpaceAction(FetchPagePort &fetchPagePort,
                          UnpinPagePort &unpinPagePort);

  TuplePageId execute(FSMPageId fsmPageId, uint32_t requiredSpace);

private:
  FetchPagePort &fetchPagePort;
  UnpinPagePort &unpinPagePort;
};
