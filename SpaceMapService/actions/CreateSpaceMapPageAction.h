#pragma once

#include "../port/NewPagePort.h"
#include "../port/UnpinPagePort.h"
#include "../port/WritePagePort.h"

class CreateSpaceMapPageAction {
public:
  CreateSpaceMapPageAction(NewPagePort &newPagePort,
                           WritePagePort &writePagePort,
                           UnpinPagePort &unpinPagePort);

  SpaceMapPageId execute();

private:
  NewPagePort &newPagePort;
  WritePagePort &writePagePort;
  UnpinPagePort &unpinPagePort;
};
