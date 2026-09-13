#pragma once

#include "../../core/Page.h"
#include "../port/PagePort.h"

class DeleteTupleAction {
public:
  explicit DeleteTupleAction(PagePort &pagePort);
  void execute(PageId pageId);
private:
  PagePort &pagePort;
};
