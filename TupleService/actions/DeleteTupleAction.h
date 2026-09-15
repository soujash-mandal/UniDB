#pragma once
#include "../../core/PageId.h"
#include "../../core/SlotId.h"
#include "../port/PagePort.h"

class DeleteTupleAction {
public:
  explicit DeleteTupleAction(PagePort &pagePort);
  void execute(const PageId &pageId, SlotId slotId);

private:
  PagePort &pagePort;
};
