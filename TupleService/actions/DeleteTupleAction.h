#pragma once

#include "../../core/PageId.h"
#include "../../core/SlotId.h"
#include "../port/FetchPagePort.h"
#include "../port/UnpinPagePort.h"

class DeleteTupleAction {
public:
  DeleteTupleAction(FetchPagePort &fetchPagePort, UnpinPagePort &unpinPagePort);

  void execute(const PageId &pageId, SlotId slotId);

private:
  FetchPagePort &fetchPagePort;
  UnpinPagePort &unpinPagePort;
};
