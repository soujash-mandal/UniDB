#pragma once

#include "../../core/PageId.h"
#include "../port/FSMFetchPagePort.h"
#include "../port/FSMUnpinPagePort.h"

class UpdateFreeSpaceAction {
public:
  UpdateFreeSpaceAction(FSMFetchPagePort &fetchPagePort,
                        FSMUnpinPagePort &unpinPagePort);

  void execute(PageId freeSpaceMapPageId, PageId dataPageId,
               uint32_t freeSpace);

private:
  FSMFetchPagePort &fetchPagePort;
  FSMUnpinPagePort &unpinPagePort;
};
