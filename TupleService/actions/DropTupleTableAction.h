#pragma once

#include "../port/FetchPagePort.h"
#include "../port/FreeTuplePagePort.h"
#include "../port/UnpinPagePort.h"

#include <cstdint>

class DropTupleTableAction {
public:
  DropTupleTableAction(TupleFetchPagePort &fetchPagePort,
                       TupleUnpinPagePort &unpinPagePort,
                       FreeTuplePagePort &freeTuplePagePort);
  void execute(uint32_t firstPageId);

private:
  TupleFetchPagePort &fetchPagePort;
  TupleUnpinPagePort &unpinPagePort;
  FreeTuplePagePort &freeTuplePagePort;
};
