#include "InsertTupleIntoTableAction.h"

#include "CreateTupleAction.h"
#include "CreateTuplePageAction.h"

#include <limits>
#include <stdexcept>

InsertTupleIntoTableAction::InsertTupleIntoTableAction(
    TupleFetchPagePort &fetchPagePort, TupleNewPagePort &newPagePort,
    TupleWritePagePort &writePagePort, TupleUnpinPagePort &unpinPagePort)
    : fetchPagePort(fetchPagePort), newPagePort(newPagePort),
      writePagePort(writePagePort), unpinPagePort(unpinPagePort) {}

TableTupleInsertResult
InsertTupleIntoTableAction::execute(TuplePageId firstPageId,
                                    const std::vector<char> &tupleData) {
  if (tupleData.empty()) {
    throw std::invalid_argument("Tuple data cannot be empty");
  }
  if (tupleData.size() > std::numeric_limits<uint32_t>::max()) {
    throw std::length_error("Tuple exceeds the V1 tuple size limit");
  }
  const auto tupleSize = static_cast<uint32_t>(tupleData.size());
  TuplePage emptyPage;
  if (!emptyPage.hasSpace(tupleSize)) {
    throw std::length_error("Tuple is too large for a tuple page");
  }

  // This action coordinates table-level page chaining; existing actions own
  // page creation and single-page tuple insertion.
  CreateTupleAction insertTuple(fetchPagePort, writePagePort, unpinPagePort);
  CreateTuplePageAction createPage(newPagePort, writePagePort, unpinPagePort);

  if (firstPageId == std::numeric_limits<TuplePageId>::max()) {
    const auto createdPage = createPage.execute();
    const TupleSlotId slotId =
        insertTuple.execute(createdPage.pageId, tupleData);
    return {createdPage.pageId, createdPage.pageId, slotId};
  }

  TuplePageId currentPageId = firstPageId;
  while (currentPageId != std::numeric_limits<TuplePageId>::max()) {
    TuplePage page = fetchPagePort.fetchPage(currentPageId);
    bool hasSpace = false;
    TuplePageId nextPageId = std::numeric_limits<TuplePageId>::max();
    try {
      hasSpace = page.hasSpace(tupleSize);
      nextPageId = page.getNextPageId();
    } catch (...) {
      try { unpinPagePort.unpinPage(currentPageId); } catch (...) {}
      throw;
    }

    if (hasSpace) {
      unpinPagePort.unpinPage(currentPageId);
      const TupleSlotId slotId = insertTuple.execute(currentPageId, tupleData);
      return {firstPageId, currentPageId, slotId};
    }

    if (nextPageId == std::numeric_limits<TuplePageId>::max()) {
      // Release the tail before allocation so a one-frame pool can evict it.
      unpinPagePort.unpinPage(currentPageId);
      const auto createdPage = createPage.execute();
      // Allocation may have evicted the old tail from a one-frame pool, so
      // fetch it again before linking and writing its next-page pointer.
      TuplePage linkPage = fetchPagePort.fetchPage(currentPageId);
      try {
        linkPage.setNextPageId(createdPage.pageId);
        writePagePort.writePage(currentPageId, linkPage);
      } catch (...) {
        try {
          unpinPagePort.unpinPage(currentPageId);
        } catch (...) {
          // Preserve the original link/write failure.
        }
        throw;
      }
      unpinPagePort.unpinPage(currentPageId);

      const TupleSlotId slotId =
          insertTuple.execute(createdPage.pageId, tupleData);
      return {firstPageId, createdPage.pageId, slotId};
    }

    unpinPagePort.unpinPage(currentPageId);
    currentPageId = nextPageId;
  }

  throw std::runtime_error("Tuple page chain is invalid");
}
