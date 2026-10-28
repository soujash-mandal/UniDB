#pragma once

#include "../port/FetchPagePort.h"
#include "../port/UnpinPagePort.h"
#include "../port/SpaceMapWritePagePort.h"

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
