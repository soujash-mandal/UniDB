#pragma once

#include "../../core/PageId.h"
#include "../port/FetchPagePort.h"
#include "../port/UnpinPagePort.h"

#include <cstdint>

class GetTupleAction {
public:
  GetTupleAction(FetchPagePort &fetchPagePort, UnpinPagePort &unpinPagePort);

  void execute(const PageId &pageId, uint16_t slotId, char *tupleData);

private:
  FetchPagePort &fetchPagePort;
  UnpinPagePort &unpinPagePort;
};
