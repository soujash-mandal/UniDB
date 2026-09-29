#pragma once

#include <vector>

#include "../domain/TuplePage.h"
#include "../port/FetchPagePort.h"
#include "../port/WritePagePort.h"

class CreateTupleAction {
public:
  CreateTupleAction(FetchPagePort &fetchPagePort, WritePagePort &writePagePort);
  TupleSlotId execute(TuplePageId pageId, std::vector<char> tupleData);

private:
  FetchPagePort &fetchPagePort;
  WritePagePort &writePagePort;
};
