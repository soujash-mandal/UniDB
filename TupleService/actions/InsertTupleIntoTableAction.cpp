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
  if (tupleData.empty())
    throw std::invalid_argument("Tuple data cannot be empty");
  if (firstPageId == std::numeric_limits<TuplePageId>::max()) {
    const TuplePageId pageId = newPagePort.newPage();
    TuplePage page;
    if (!page.hasSpace(static_cast<uint32_t>(tupleData.size()))) {
      unpinPagePort.unpinPage(pageId);
      throw std::length_error("Tuple is too large for a tuple page");
    }
    const TupleSlotId slotId = page.insert(tupleData);
    writePagePort.writePage(pageId, page);
    unpinPagePort.unpinPage(pageId);
    return {pageId, pageId, slotId};
  }
  TuplePageId currentPageId = firstPageId;
  while (currentPageId != std::numeric_limits<TuplePageId>::max()) {
    TuplePage page = fetchPagePort.fetchPage(currentPageId);
    if (page.hasSpace(static_cast<uint32_t>(tupleData.size()))) {
      const TupleSlotId slotId = page.insert(tupleData);
      writePagePort.writePage(currentPageId, page);
      unpinPagePort.unpinPage(currentPageId);
      return {firstPageId, currentPageId, slotId};
    }
    const TuplePageId nextPageId = page.getNextPageId();
    if (nextPageId == std::numeric_limits<TuplePageId>::max()) {
      const TuplePageId newPageId = newPagePort.newPage();
      TuplePage newPage;
      if (!newPage.hasSpace(static_cast<uint32_t>(tupleData.size()))) {
        unpinPagePort.unpinPage(newPageId);
        unpinPagePort.unpinPage(currentPageId);
        throw std::length_error("Tuple is too large for a tuple page");
      }
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
