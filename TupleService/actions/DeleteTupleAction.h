#pragma once

#include "../domain/TuplePage.h"
#include "../port/FetchPagePort.h"
#include "../port/UnpinPagePort.h"
#include "../port/WritePagePort.h"

class DeleteTupleAction {
public:
  DeleteTupleAction(TupleFetchPagePort &fetchPagePort,
                    TupleWritePagePort &writePagePort,
                    TupleUnpinPagePort &unpinPagePort);
  void execute(TuplePageId pageId, TupleSlotId slotId);

private:
  TupleFetchPagePort &fetchPagePort;
  TupleWritePagePort &writePagePort;
  TupleUnpinPagePort &unpinPagePort;
};
