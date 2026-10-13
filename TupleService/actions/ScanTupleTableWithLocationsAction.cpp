#include "ScanTupleTableWithLocationsAction.h"
#include <limits>
ScanTupleTableWithLocationsAction::ScanTupleTableWithLocationsAction(TupleFetchPagePort &fetch, TupleUnpinPagePort &unpin)
  : fetchPagePort(fetch), unpinPagePort(unpin) {}
std::vector<LocatedTuple> ScanTupleTableWithLocationsAction::execute(TuplePageId firstPageId) {
  std::vector<LocatedTuple> result;
  auto pageId = firstPageId;
  while (pageId != std::numeric_limits<TuplePageId>::max()) {
    auto page = fetchPagePort.fetchPage(pageId);
    const auto next = page.getNextPageId();
    for (auto &record : page.getAllTupleRecords())
      result.push_back({pageId, record.slotId, std::move(record.data)});
    unpinPagePort.unpinPage(pageId);
    pageId = next;
  }
  return result;
}