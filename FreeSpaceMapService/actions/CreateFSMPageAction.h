#pragma once

#include "../port/NewPagePort.h"
#include "../port/UnpinPagePort.h"
#include "../port/WritePagePort.h"

class CreateFSMPageAction {
public:
  CreateFSMPageAction(NewPagePort &newPagePort, WritePagePort &writePagePort,
                      UnpinPagePort &unpinPagePort);

  FSMPageId execute();

private:
  NewPagePort &newPagePort;
  WritePagePort &writePagePort;
  UnpinPagePort &unpinPagePort;
};
