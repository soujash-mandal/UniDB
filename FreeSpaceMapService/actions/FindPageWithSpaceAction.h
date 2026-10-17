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

  // Returns an invalid TuplePageId when no existing tuple page has enough space.
  // RdbService owns creating a tuple page and registering its free space.
  TuplePageId execute(FSMPageId rootFsmPageId, uint32_t requiredSpace);

private:
  FetchPagePort &fetchPagePort;
  UnpinPagePort &unpinPagePort;
  WritePagePort &writePagePort;
  NewPagePort &newPagePort;
};
