#pragma once

#include "../port/NewPagePort.h"
#include "../port/UnpinPagePort.h"
#include "../port/SpaceMapWritePagePort.h"

class CreateSpaceMapPageAction {
public:
  CreateSpaceMapPageAction(NewPagePort &newPagePort,
                           SpaceMapWritePagePort &writePagePort,
                           UnpinPagePort &unpinPagePort);

  SpaceMapPageId execute();

private:
  NewPagePort &newPagePort;
  SpaceMapWritePagePort &writePagePort;
  UnpinPagePort &unpinPagePort;
};
