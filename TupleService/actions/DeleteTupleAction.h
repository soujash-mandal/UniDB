#pragma once

#include "../port/FetchPagePort.h"
#include "../port/WritePagePort.h"

class DeleteTupleAction {
public:
  DeleteTupleAction(TupleFetchPagePort &fetchPagePort, TupleWritePagePort &writePagePort);
  void execute(TuplePageId pageId, TupleSlotId slotId);
private:
  TupleFetchPagePort &fetchPagePort;
  TupleWritePagePort &writePagePort;
};
