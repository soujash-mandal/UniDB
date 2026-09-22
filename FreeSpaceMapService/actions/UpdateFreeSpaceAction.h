#pragma once

#include "../../core/PageId.h"
#include "../port/FetchPagePort.h"
#include "../port/UnpinPagePort.h"

class UpdateFreeSpaceAction {
public:
  UpdateFreeSpaceAction(FetchPagePort &fetchPagePort,
                        UnpinPagePort &unpinPagePort);

  void execute(PageId freeSpaceMapPageId, PageId dataPageId,
               uint32_t freeSpace);

private:
  FetchPagePort &fetchPagePort;
  UnpinPagePort &unpinPagePort;
};
