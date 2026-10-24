#include "GetAllTuplesAction.h"

GetAllTuplesAction::GetAllTuplesAction(TupleFetchPagePort &fetchPagePort,
                                       TupleUnpinPagePort &unpinPagePort)
    : fetchPagePort(fetchPagePort), unpinPagePort(unpinPagePort) {}

std::vector<std::vector<char>> GetAllTuplesAction::execute(TuplePageId pageId) {
  TuplePage page = fetchPagePort.fetchPage(pageId);
  std::vector<std::vector<char>> tuples;

  try {
    tuples = page.getAllTuples();
  } catch (...) {
    try {
      unpinPagePort.unpinPage(pageId);
    } catch (...) {
      // Preserve the original scan failure.
    }
    throw;
  }

  unpinPagePort.unpinPage(pageId);
  return tuples;
}
