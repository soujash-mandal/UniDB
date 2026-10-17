#include "FindPageWithSpaceAction.h"

#include <limits>
#include <stdexcept>
#include <unordered_set>

FindPageWithSpaceAction::FindPageWithSpaceAction(FetchPagePort &fetchPagePort,
                                                UnpinPagePort &unpinPagePort,
                                                WritePagePort &writePagePort,
                                                NewPagePort &newPagePort)
    : fetchPagePort(fetchPagePort), unpinPagePort(unpinPagePort),
      writePagePort(writePagePort), newPagePort(newPagePort) {}

TuplePageId FindPageWithSpaceAction::execute(FSMPageId rootFsmPageId,
                                             uint32_t requiredSpace) {
  const TuplePageId invalidTuplePageId =
      std::numeric_limits<TuplePageId>::max();
  FSMPageId fsmPageId = rootFsmPageId;
  std::unordered_set<FSMPageId> visitedPageIds;

  while (FSMPage::isValidPage(fsmPageId)) {
    if (!visitedPageIds.insert(fsmPageId).second) {
      throw std::runtime_error("Free-space-map page chain is invalid");
    }

    FSMPage fsmPage = fetchPagePort.fetchPage(fsmPageId);
    TuplePageId tuplePageId;
    FSMPageId nextFsmPageId;
    bool fsmPageIsFull;
    try {
      tuplePageId = fsmPage.findPageWithSpace(requiredSpace);
      nextFsmPageId = fsmPage.getNextPageId();
      fsmPageIsFull = fsmPage.isFull();
    } catch (...) {
      try {
        unpinPagePort.unpinPage(fsmPageId);
      } catch (...) {
        // Preserve the original page-inspection failure.
      }
      throw;
    }

    unpinPagePort.unpinPage(fsmPageId);
    if (FSMPage::isValidPage(tuplePageId)) {
      return tuplePageId;
    }
    if (FSMPage::isValidPage(nextFsmPageId)) {
      fsmPageId = nextFsmPageId;
      continue;
    }
    if (!fsmPageIsFull) {
      return invalidTuplePageId;
    }

    // Extend only the FSM chain here. RdbService creates tuple pages.
    const FSMPageId newFsmPageId = newPagePort.newPage();
    FSMPage newFsmPage;
    try {
      writePagePort.writePage(newFsmPageId, newFsmPage);
    } catch (...) {
      try {
        unpinPagePort.unpinPage(newFsmPageId);
      } catch (...) {
        // Preserve the original page initialization failure.
      }
      throw;
    }
    unpinPagePort.unpinPage(newFsmPageId);

    FSMPage tailPage = fetchPagePort.fetchPage(fsmPageId);
    try {
      tailPage.setNextPageId(newFsmPageId);
      writePagePort.writePage(fsmPageId, tailPage);
    } catch (...) {
      try {
        unpinPagePort.unpinPage(fsmPageId);
      } catch (...) {
        // Preserve the original link/write failure.
      }
      throw;
    }
    unpinPagePort.unpinPage(fsmPageId);
    fsmPageId = newFsmPageId;
  }

  return invalidTuplePageId;
}
