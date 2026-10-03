#include "FindPageWithSpaceAction.h"

FindPageWithSpaceAction::FindPageWithSpaceAction(FetchPagePort &fetchPagePort,
                                                 UnpinPagePort &unpinPagePort)
    : fetchPagePort(fetchPagePort), unpinPagePort(unpinPagePort) {}

TuplePageId FindPageWithSpaceAction::execute(FSMPageId rootFsmPageId,
                                             uint32_t requiredSpace) {

  FSMPageId fsmPageId = rootFsmPageId;
  while (fsmPageId != INVALID_PAGE_ID) {
    FSMPage page = fetchPagePort.fetchPage(fsmPageId);
    TuplePageId tuplePageId = page.findPageWithSpace(requiredSpace);

    if (tuplePageId == INVALID_PAGE_ID) {
      if (page.isFull()) {
        FSMPageId nextPageId = page.getNextPageId();
        if (nextPageId == INVALID_PAGE_ID) {
          // todo: create new fsm page, create a new tuple page
        } else {
          unpinPagePort.unpinPage(fsmPageId);
          fsmPageId = nextPageId;
        }
      } else {
        // todo: create new tuple page and assign in that page
      }
    } else {
      unpinPagePort.unpinPage(fsmPageId);
      return tuplePageId;
    }
  }
  throw "error";
}
