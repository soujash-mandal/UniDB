#include "ScanTupleTableAction.h"

#include <limits>

ScanTupleTableAction::ScanTupleTableAction(TupleFetchPagePort &fetchPagePort,
                                           TupleUnpinPagePort &unpinPagePort)
    : fetchPagePort(fetchPagePort), unpinPagePort(unpinPagePort) {}

std::vector<std::vector<char>>
ScanTupleTableAction::execute(TuplePageId firstPageId) {
  std::vector<std::vector<char>> tuples;
  TuplePageId currentPageId = firstPageId;
  while (currentPageId != std::numeric_limits<TuplePageId>::max()) {
    TuplePage page = fetchPagePort.fetchPage(currentPageId);
    TuplePageId nextPageId;
    try {
      nextPageId = page.getNextPageId();
      auto pageTuples = page.getAllTuples();
      tuples.insert(tuples.end(), pageTuples.begin(), pageTuples.end());
    } catch (...) {
      try {
        unpinPagePort.unpinPage(currentPageId);
      } catch (...) {
        // Preserve the original materialization failure.
      }
      throw;
    }
    unpinPagePort.unpinPage(currentPageId);
    currentPageId = nextPageId;
  }
  return tuples;
}
