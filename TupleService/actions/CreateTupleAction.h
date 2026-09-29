#pragma once

#include "../domain/TuplePage.h"
#include "../port/FetchPagePort.h"
#include "../port/UnpinPagePort.h"

#include <vector>

class CreateTupleAction {
public:
  CreateTupleAction(FetchPagePort &fetchPagePort, UnpinPagePort &unpinPagePort);
  TupleSlotId execute(TuplePageId pageId, std::vector<char> tupleData);

private:
  FetchPagePort &fetchPagePort;
  UnpinPagePort &unpinPagePort;
};
