#pragma once

#include "../port/NewPagePort.h"
#include "../port/UnpinPagePort.h"
#include "../port/WritePagePort.h"

class InitializeFSMPageAction {
public:
  InitializeFSMPageAction(NewPagePort &newPagePort,
                          WritePagePort &writePagePort,
                          UnpinPagePort &unpinPagePort);

  FSMPageId execute();

private:
  NewPagePort &newPagePort;
  WritePagePort &writePagePort;
  UnpinPagePort &unpinPagePort;
};
