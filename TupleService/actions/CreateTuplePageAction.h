#pragma once

#include "../port/NewPagePort.h"
#include "../port/UnpinPagePort.h"
#include "../port/WritePagePort.h"

class CreateTuplePageAction {
public:
  explicit CreateTuplePageAction(NewPagePort &newPagePort,
                                 WritePagePort &writePagePort,
                                 UnpinPagePort &unpinPagePort);
  TuplePageId execute();

private:
  NewPagePort &newPagePort;
  WritePagePort &writePagePort;
  UnpinPagePort &unpinPagePort;
};
