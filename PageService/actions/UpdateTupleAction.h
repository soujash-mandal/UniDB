#pragma once

#include "../../core/Page.h"
#include "../port/PagePort.h"

class UpdateTupleAction {
public:
  explicit UpdateTupleAction(PagePort &pagePort);
  void execute(PageId pageId, const char *tupleData);

private:
  PagePort &pagePort;
};
