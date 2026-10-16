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
  TuplePage emptyPage;
  if (!emptyPage.hasSpace(static_cast<uint32_t>(tupleData.size()))) {
    throw std::length_error("Tuple is too large for a tuple page");
  }

  // Keep page allocation and single-page tuple insertion in their existing
  // actions; this action only coordinates table-level page chaining.
  CreateTupleAction insertTuple(fetchPagePort, writePagePort, unpinPagePort);
  CreateTuplePageAction createPage(newPagePort, writePagePort, unpinPagePort);

  if (firstPageId == std::numeric_limits<TuplePageId>::max()) {
    const auto createdPage = createPage.execute();
    const TupleSlotId slotId = insertTuple.execute(createdPage.pageId, tupleData);
    return {createdPage.pageId, createdPage.pageId, slotId};
  }

  TuplePageId currentPageId = firstPageId;
  while (currentPageId != std::numeric_limits<TuplePageId>::max()) {
    TuplePage page = fetchPagePort.fetchPage(currentPageId);
    if (page.hasSpace(static_cast<uint32_t>(tupleData.size()))) {
      unpinPagePort.unpinPage(currentPageId);
      const TupleSlotId slotId = insertTuple.execute(currentPageId, tupleData);
      return {firstPageId, currentPageId, slotId};
    }

    const TuplePageId nextPageId = page.getNextPageId();
    if (nextPageId == std::numeric_limits<TuplePageId>::max()) {
      // A tuple table's page chain is distinct from the allocator's free-page
      // bookkeeping: link the new page so scans can find it from the root.
      const auto createdPage = createPage.execute();
      page.setNextPageId(createdPage.pageId);
      writePagePort.writePage(currentPageId, page);
      unpinPagePort.unpinPage(currentPageId);

      const TupleSlotId slotId = insertTuple.execute(createdPage.pageId, tupleData);
      return {firstPageId, createdPage.pageId, slotId};
    }

    unpinPagePort.unpinPage(currentPageId);
    currentPageId = nextPageId;
  }

  throw std::runtime_error("Tuple page chain is invalid");
}
