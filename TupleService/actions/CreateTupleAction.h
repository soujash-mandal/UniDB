#pragma once

#include <vector>
#include "../domain/TuplePage.h"
#include "../port/FetchPagePort.h"
#include "../port/WritePagePort.h"

class CreateTupleAction {
public:
  CreateTupleAction(TupleFetchPagePort &fetchPagePort, TupleWritePagePort &writePagePort);
  TupleSlotId execute(TuplePageId pageId, std::vector<char> tupleData);
private:
  TupleFetchPagePort &fetchPagePort;
  TupleWritePagePort &writePagePort;
};
