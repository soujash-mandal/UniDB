#include "InsertTupleIntoTableAction.h"

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
    throw std::length_error("Tuple is too large for a tuple page");
  }

  const auto tupleSize = static_cast<uint32_t>(tupleData.size());
  TuplePage emptyPage;
  if (!emptyPage.hasSpace(tupleSize)) {
    throw std::length_error("Tuple is too large for a tuple page");
  }

  if (firstPageId == std::numeric_limits<TuplePageId>::max()) {
    const TuplePageId pageId = newPagePort.newPage();
    TuplePage page;
    const TupleSlotId slotId = page.insert(tupleData);
    writePagePort.writePage(pageId, page);
    unpinPagePort.unpinPage(pageId);
    return {pageId, pageId, slotId};
  }

  TuplePageId currentPageId = firstPageId;
  while (currentPageId != std::numeric_limits<TuplePageId>::max()) {
    TuplePage page = fetchPagePort.fetchPage(currentPageId);
    if (page.hasSpace(tupleSize)) {
      const TupleSlotId slotId = page.insert(tupleData);
      writePagePort.writePage(currentPageId, page);
      unpinPagePort.unpinPage(currentPageId);
      return {firstPageId, currentPageId, slotId};
    }

    const TuplePageId nextPageId = page.getNextPageId();
    if (nextPageId == std::numeric_limits<TuplePageId>::max()) {
      TuplePageId newPageId;
      try {
        newPageId = newPagePort.newPage();
      } catch (...) {
        unpinPagePort.unpinPage(currentPageId);
        throw;
      }

      TuplePage newPage;
      page.setNextPageId(newPageId);
      writePagePort.writePage(currentPageId, page);
      const TupleSlotId slotId = newPage.insert(tupleData);
      writePagePort.writePage(newPageId, newPage);
      unpinPagePort.unpinPage(newPageId);
      unpinPagePort.unpinPage(currentPageId);
      return {firstPageId, newPageId, slotId};
    }

    unpinPagePort.unpinPage(currentPageId);
    currentPageId = nextPageId;
  }

  throw std::runtime_error("Tuple page chain is invalid");
}
