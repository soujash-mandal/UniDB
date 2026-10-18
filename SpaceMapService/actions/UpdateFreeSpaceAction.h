#pragma once

#include "../port/FetchPagePort.h"
#include "../port/UnpinPagePort.h"
#include "../port/WritePagePort.h"

class UpdateFreeSpaceAction {
public:
  UpdateFreeSpaceAction(FetchPagePort &fetchPagePort,
                        WritePagePort &writePagePort,
                        UnpinPagePort &unpinPagePort);

  void execute(FSMPageId fsmPageId, TuplePageId pageId,
               uint32_t updatedfFreeSpace);

private:
  FetchPagePort &fetchPagePort;
  WritePagePort &writePagePort;
  UnpinPagePort &unpinPagePort;
};
