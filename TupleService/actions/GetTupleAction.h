#pragma once

#include "../domain/TuplePage.h"
#include "../port/FetchPagePort.h"
#include "../port/UnpinPagePort.h"

#include <vector>

class GetTupleAction {
public:
  GetTupleAction(FetchPagePort &fetchPagePort, UnpinPagePort &unpinPagePort);
  std::vector<char> execute(TuplePageId pageId, TupleSlotId slotId);

private:
  FetchPagePort &fetchPagePort;
  UnpinPagePort &unpinPagePort;
};
