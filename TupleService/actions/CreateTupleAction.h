#pragma once

#include "../../core/PageId.h"
#include "../port/FetchPagePort.h"
#include "../port/UnpinPagePort.h"
#include <cstdint>

class CreateTupleAction {
public:
  CreateTupleAction(FetchPagePort &fetchPagePort, UnpinPagePort &unpinPagePort);

  uint16_t execute(const PageId &pageId, const char *tupleData,
                   uint16_t tupleSize);

private:
  FetchPagePort &fetchPagePort;
  UnpinPagePort &unpinPagePort;
};
