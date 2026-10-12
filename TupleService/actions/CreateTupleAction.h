#pragma once

#include "../domain/TuplePage.h"
#include "../port/FetchPagePort.h"
#include "../port/WritePagePort.h"
#include "../port/UnpinPagePort.h"
#include <vector>

class CreateTupleAction {
public:
  CreateTupleAction(TupleFetchPagePort &fetchPagePort,
                    TupleWritePagePort &writePagePort,
                    TupleUnpinPagePort &unpinPagePort);
  TupleSlotId execute(TuplePageId pageId, std::vector<char> tupleData);

private:
  TupleFetchPagePort &fetchPagePort;
  TupleWritePagePort &writePagePort;
  TupleUnpinPagePort &unpinPagePort;
};
