#pragma once

#include "../port/FetchPagePort.h"
#include "../port/WritePagePort.h"
#include "../port/UnpinPagePort.h"

class DeleteTupleAction {
public:
  DeleteTupleAction(TupleFetchPagePort &fetchPagePort,
                    TupleWritePagePort &writePagePort);
  DeleteTupleAction(TupleFetchPagePort &fetchPagePort,
                    TupleWritePagePort &writePagePort,
                    TupleUnpinPagePort &unpinPagePort);
  void execute(TuplePageId pageId, TupleSlotId slotId);

private:
  TupleFetchPagePort &fetchPagePort;
  TupleWritePagePort &writePagePort;
  TupleUnpinPagePort *unpinPagePort = nullptr;
};
