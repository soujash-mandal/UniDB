#pragma once

#include <cstdint>

#include "../../core/PageId.h"
#include "../../core/TableId.h"
#include "../port/FetchPagePort.h"
#include "../port/UnpinPagePort.h"

class FindPageWithSpaceAction {
public:
  FindPageWithSpaceAction(FetchPagePort &fetchPagePort,
                          UnpinPagePort &unpinPagePort);

  PageId execute(TableId tableId, PageId firstFreeSpaceMapPageId,
                 uint32_t requiredSpace);

private:
  FetchPagePort &fetchPagePort;
  UnpinPagePort &unpinPagePort;
};
