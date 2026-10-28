#pragma once

#include "../port/FetchPagePort.h"
#include "../port/UnpinPagePort.h"
#include "../port/WritePagePort.h"

class UpdateFreeSpaceAction {
public:
  UpdateFreeSpaceAction(FetchPagePort &fetchPagePort,
                        SpaceMapWritePagePort &writePagePort,
                        UnpinPagePort &unpinPagePort);

  void execute(SpaceMapPageId spaceMapPageId, TuplePageId pageId,
               uint32_t updatedfFreeSpace);

private:
  FetchPagePort &fetchPagePort;
  SpaceMapWritePagePort &writePagePort;
  UnpinPagePort &unpinPagePort;
};
