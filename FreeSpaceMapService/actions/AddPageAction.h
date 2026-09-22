#pragma once

#include "../../core/PageId.h"
#include "../../core/TableId.h"
#include "../port/FetchPagePort.h"
#include "../port/NewPagePort.h"
#include "../port/UnpinPagePort.h"

class AddPageAction {
public:
  AddPageAction(FetchPagePort &fetchPagePort, NewPagePort &newPagePort,
                UnpinPagePort &unpinPagePort);

  void execute(TableId tableId, PageId freeSpaceMapPageId, PageId dataPageId,
               uint32_t freeSpace);

private:
  FetchPagePort &fetchPagePort;
  NewPagePort &newPagePort;
  UnpinPagePort &unpinPagePort;
};
