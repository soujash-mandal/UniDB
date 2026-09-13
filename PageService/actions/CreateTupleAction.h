#pragma once

#include "../../core/Page.h"
#include "../port/PagePort.h"

class CreateTupleAction {
public:
  explicit CreateTupleAction(PagePort &pagePort);
  void execute(PageId pageId, const char *tupleData);

private:
  PagePort &pagePort;
};
