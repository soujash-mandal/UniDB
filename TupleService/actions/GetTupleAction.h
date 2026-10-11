#pragma once

#include "../domain/TuplePage.h"
#include "../port/FetchPagePort.h"
#include "../port/UnpinPagePort.h"
#include <vector>

class GetTupleAction {
public:
  GetTupleAction(TupleFetchPagePort &fetchPagePort,
                 TupleUnpinPagePort &unpinPagePort);
  std::vector<char> execute(TuplePageId pageId, TupleSlotId slotId);

private:
  TupleFetchPagePort &fetchPagePort;
  TupleUnpinPagePort &unpinPagePort;
};
