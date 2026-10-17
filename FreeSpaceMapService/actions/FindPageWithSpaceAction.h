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
                          NewPagePort &newPagePort)
      : fetchPagePort(fetchPagePort), unpinPagePort(unpinPagePort),
        writePagePort(writePagePort), newPagePort(newPagePort) {}

  // Returns an invalid TuplePageId if no existing tuple page has enough space.
  // RdbService creates the tuple page and registers its free-space metadata.
  TuplePageId execute(FSMPageId rootFsmPageId, uint32_t requiredSpace);

private:
  FetchPagePort &fetchPagePort;
  UnpinPagePort &unpinPagePort;
  WritePagePort &writePagePort;
  NewPagePort &newPagePort;
};
