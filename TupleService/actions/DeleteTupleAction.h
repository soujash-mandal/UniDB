#pragma once

#include "../domain/TuplePage.h"
#include "../port/FetchPagePort.h"
#include "../port/UnpinPagePort.h"

class DeleteTupleAction {
public:
  DeleteTupleAction(FetchPagePort &fetchPagePort, UnpinPagePort &unpinPagePort);
  void execute(TuplePageId pageId, TupleSlotId slotId);

private:
  FetchPagePort &fetchPagePort;
  UnpinPagePort &unpinPagePort;
};
