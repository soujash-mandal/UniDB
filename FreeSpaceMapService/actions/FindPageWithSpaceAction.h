#pragma once

#include <cstdint>

#include "../../core/PageId.h"
#include "../../core/TableId.h"
#include "../port/FSMFetchPagePort.h"
#include "../port/FSMUnpinPagePort.h"

class FindPageWithSpaceAction {
public:
  FindPageWithSpaceAction(FSMFetchPagePort &fetchPagePort,
                          FSMUnpinPagePort &unpinPagePort);

  PageId execute(TableId tableId, PageId firstFreeSpaceMapPageId,
                 uint32_t requiredSpace);

private:
  FSMFetchPagePort &fetchPagePort;
  FSMUnpinPagePort &unpinPagePort;
};
