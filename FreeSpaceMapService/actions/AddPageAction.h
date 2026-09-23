#pragma once

#include "../../core/PageId.h"
#include "../../core/TableId.h"
#include "../port/FSMFetchPagePort.h"
#include "../port/FSMNewPagePort.h"
#include "../port/FSMUnpinPagePort.h"

class AddPageAction {
public:
  AddPageAction(FSMFetchPagePort &fetchPagePort,
                FSMUnpinPagePort &unpinPagePort, FSMNewPagePort &newPagePort);

  void execute(TableId tableId, PageId freeSpaceMapPageId, PageId dataPageId,
               uint32_t freeSpace);

private:
  FSMFetchPagePort &fetchPagePort;
  FSMUnpinPagePort &unpinPagePort;
  FSMNewPagePort &newPagePort;
};
