#pragma once

#include "../port/FetchPagePort.h"
#include "../port/NewPagePort.h"
#include "../port/UnpinPagePort.h"
#include "../port/WritePagePort.h"

#include <vector>

struct TableTupleInsertResult {
  TuplePageId firstPageId;
  TuplePageId pageId;
  TupleSlotId slotId;
};

class InsertTupleIntoTableAction {
public:
  InsertTupleIntoTableAction(FetchPagePort &fetchPagePort,
                             NewPagePort &newPagePort,
                             WritePagePort &writePagePort,
                             UnpinPagePort &unpinPagePort);

  TableTupleInsertResult execute(TuplePageId firstPageId,
                                 const std::vector<char> &tupleData);

private:
  FetchPagePort &fetchPagePort;
  NewPagePort &newPagePort;
  WritePagePort &writePagePort;
  UnpinPagePort &unpinPagePort;
};
