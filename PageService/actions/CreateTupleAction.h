#pragma once

#include "../../core/PageId.h"
#include "../port/PagePort.h"
#include <cstdint>

class CreateTupleAction {
public:
  explicit CreateTupleAction(PagePort &pagePort);
  uint16_t execute(const PageId &pageId, const char *tupleData,
                   uint16_t tupleSize);

private:
  PagePort &pagePort;
};
