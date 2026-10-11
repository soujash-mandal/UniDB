#pragma once

#include "../domain/TuplePage.h"
#include "../port/FetchPagePort.h"
#include "../port/WritePagePort.h"
#include <vector>

class CreateTupleAction {
public:
  CreateTupleAction(TupleFetchPagePort &fetchPagePort,
                    TupleWritePagePort &writePagePort);
  TupleSlotId execute(TuplePageId pageId, std::vector<char> tupleData);

private:
  TupleFetchPagePort &fetchPagePort;
  TupleWritePagePort &writePagePort;
};
