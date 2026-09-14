#pragma once

#include "../../core/PageId.h"
#include "../port/PagePort.h"
#include <cstdint>

class ReadTupleAction {
public:
  explicit ReadTupleAction(PagePort &pagePort);
  void execute(const PageId &pageId, uint16_t slotId, char *tupleData);

private:
  PagePort &pagePort;
};
