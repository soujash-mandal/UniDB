#pragma once

#include "../domain/Page.h"
#include "../port/PagePort.h"

class ReadTupleAction {
public:
  explicit ReadTupleAction(PagePort &pagePort);
  void execute(PageId pageId, Page &page);

private:
  PagePort &pagePort;
};
