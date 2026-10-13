#pragma once
#include "../port/FetchPagePort.h"
#include "../port/UnpinPagePort.h"
#include "../domain/TuplePage.h"
#include <vector>
struct LocatedTuple {
  TuplePageId pageId;
  TupleSlotId slotId;
  std::vector<char> data;
};
class ScanTupleTableWithLocationsAction {
public:
  ScanTupleTableWithLocationsAction(TupleFetchPagePort &, TupleUnpinPagePort &);
  std::vector<LocatedTuple> execute(TuplePageId firstPageId);
private:
  TupleFetchPagePort &fetchPagePort;
  TupleUnpinPagePort &unpinPagePort;
};