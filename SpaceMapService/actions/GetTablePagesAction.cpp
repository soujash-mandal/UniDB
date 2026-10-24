#include "GetTablePagesAction.h"

#include <stdexcept>
#include <unordered_set>

GetTablePagesAction::GetTablePagesAction(FetchPagePort &fetchPagePort,
                                         UnpinPagePort &unpinPagePort)
    : fetchPagePort(fetchPagePort), unpinPagePort(unpinPagePort) {}

std::vector<TuplePageId>
GetTablePagesAction::execute(SpaceMapPageId rootSpaceMapPageId) {
  std::vector<TuplePageId> tuplePageIds;
  if (!SpaceMapPage::isValidPage(rootSpaceMapPageId)) {
    return tuplePageIds;
  }

  std::unordered_set<SpaceMapPageId> visitedSpaceMapPages;
  SpaceMapPageId currentPageId = rootSpaceMapPageId;

  while (SpaceMapPage::isValidPage(currentPageId)) {
    if (!visitedSpaceMapPages.insert(currentPageId).second) {
      throw std::runtime_error("Cycle detected in SpaceMap page chain");
    }

    // A failed fetch has not returned a pinned page to this action.
    SpaceMapPage page = fetchPagePort.fetchPage(currentPageId);
    bool pagePinned = true;
    try {
      std::vector<TuplePageId> pageTupleIds = page.getPageIds();
      tuplePageIds.insert(tuplePageIds.end(), pageTupleIds.begin(),
                          pageTupleIds.end());
      const SpaceMapPageId nextPageId = page.getNextPageId();

      // Do not retry unpin if the port itself reports an unpin failure.
      pagePinned = false;
      unpinPagePort.unpinPage(currentPageId);
      currentPageId = nextPageId;
    } catch (...) {
      if (pagePinned) {
        try {
          unpinPagePort.unpinPage(currentPageId);
        } catch (...) {
          // Preserve the original processing exception.
        }
      }
      throw;
    }
  }

  return tuplePageIds;
}
