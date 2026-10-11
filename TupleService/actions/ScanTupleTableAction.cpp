#include "ScanTupleTableAction.h"

#include <limits>
#include <stdexcept>

ScanTupleTableAction::ScanTupleTableAction(FetchPagePort &fetchPagePort,
                                           UnpinPagePort &unpinPagePort)
    : fetchPagePort(fetchPagePort), unpinPagePort(unpinPagePort) {}

std::vector<std::vector<char>>
ScanTupleTableAction::execute(TuplePageId firstPageId) {
  std::vector<std::vector<char>> tuples;
  TuplePageId currentPageId = firstPageId;
  while (currentPageId != std::numeric_limits<TuplePageId>::max()) {
    TuplePage page = fetchPagePort.fetchPage(currentPageId);
    const TuplePageId nextPageId = page.getNextPageId();
    auto pageTuples = page.getAllTuples();
    tuples.insert(tuples.end(), pageTuples.begin(), pageTuples.end());
    unpinPagePort.unpinPage(currentPageId);
    currentPageId = nextPageId;
  }
  return tuples;
}
