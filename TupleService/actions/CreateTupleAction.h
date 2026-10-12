#pragma once

#include <vector>

#include "../domain/TuplePage.h"
#include "../port/FetchPagePort.h"
#include "../port/UnpinPagePort.h"
#include "../port/WritePagePort.h"

class CreateTupleAction {
public:
  CreateTupleAction(FetchPagePort &fetchPagePort, WritePagePort &writePagePort,
                    UnpinPagePort &unpinPagePort);
  TupleSlotId execute(TuplePageId pageId, std::vector<char> tupleData);

private:
  FetchPagePort &fetchPagePort;
  WritePagePort &writePagePort;
  UnpinPagePort &unpinPagePort;
};
