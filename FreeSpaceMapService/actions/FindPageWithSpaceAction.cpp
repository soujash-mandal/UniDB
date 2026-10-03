#include "FindPageWithSpaceAction.h"

FindPageWithSpaceAction::FindPageWithSpaceAction(FetchPagePort &fetchPagePort,
                                                 UnpinPagePort &unpinPagePort,
                                                 WritePagePort &writePagePort,
                                                 NewPagePort &newPagePort)
    : fetchPagePort(fetchPagePort), unpinPagePort(unpinPagePort),
      writePagePort(writePagePort), newPagePort(newPagePort) {}

TuplePageId FindPageWithSpaceAction::execute(FSMPageId rootFsmPageId,
                                             uint32_t requiredSpace) {

  FSMPageId fsmPageId = rootFsmPageId;

  while (fsmPageId != INVALID_PAGE_ID) {

    FSMPage fsmPage = fetchPagePort.fetchPage(fsmPageId);
    TuplePageId tuplePageId = fsmPage.findPageWithSpace(requiredSpace);

    if (tuplePageId == INVALID_PAGE_ID) {
      if (fsmPage.isFull()) {
        FSMPageId nextFsmPageId = fsmPage.getNextPageId();
        if (nextFsmPageId == INVALID_PAGE_ID) {
          nextFsmPageId = newPagePort.newPage();
          fsmPage.setNextPageId(nextFsmPageId);
          TuplePageId tuplePageId = newPagePort.newPage();
          fsmPage.insert(tuplePageId, freeSpace);
          writePagePort.writePage(fsmPageId, fsmPage);
          unpinPagePort.unpinPage(fsmPageId);
          return tuplePageId;
        } else {
          unpinPagePort.unpinPage(fsmPageId);
          fsmPageId = nextFsmPageId;
        }
      } else {
        TuplePageId tuplePageId = newPagePort.newPage();
        fsmPage.insert(tuplePageId, freeSpace);
        unpinPagePort.unpinPage(fsmPageId);
        return tuplePageId;
      }
    } else {
      unpinPagePort.unpinPage(fsmPageId);
      return tuplePageId;
    }
  }
  throw "error";
}
