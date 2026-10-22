#include "ScanTupleTableAction.h"

ScanTupleTableAction::ScanTupleTableAction(TupleFetchPagePort &fetchPagePort,
                                           TupleUnpinPagePort &unpinPagePort)
    : fetchPagePort(fetchPagePort), unpinPagePort(unpinPagePort) {}

std::vector<std::vector<char>>
ScanTupleTableAction::execute(const std::vector<TuplePageId> &tuplePageIds) {
  std::vector<std::vector<char>> tuples;
  for (const TuplePageId pageId : tuplePageIds) {
    TuplePage page = fetchPagePort.fetchPage(pageId);
    try {
      auto pageTuples = page.getAllTuples();
      tuples.insert(tuples.end(), pageTuples.begin(), pageTuples.end());
    } catch (...) {
      try {
        unpinPagePort.unpinPage(pageId);
      } catch (...) {
        // Preserve the original materialization failure.
      }
      throw;
    }
    unpinPagePort.unpinPage(pageId);
  }
  return tuples;
}
