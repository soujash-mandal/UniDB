#pragma once

#include <vector>

#include "../domain/TuplePage.h"
#include "../port/FetchPagePort.h"
#include "../port/UnpinPagePort.h"

class GetTupleAction {
public:
  GetTupleAction(TupleFetchPagePort &fetchPagePort,
                 TupleUnpinPagePort &unpinPagePort);
  std::vector<char> execute(TuplePageId pageId, TupleSlotId slotId);

private:
  TupleFetchPagePort &fetchPagePort;
  TupleUnpinPagePort &unpinPagePort;
};
