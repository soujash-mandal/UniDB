#pragma once

#include "../domain/TuplePage.h"
#include "../port/FetchPagePort.h"
#include "../port/WritePagePort.h"

class DeleteTupleAction {
public:
  DeleteTupleAction(FetchPagePort &fetchPagePort, WritePagePort &writePagePort);
  void execute(TuplePageId pageId, TupleSlotId slotId);

private:
  FetchPagePort &fetchPagePort;
  WritePagePort &writePagePort;
};
