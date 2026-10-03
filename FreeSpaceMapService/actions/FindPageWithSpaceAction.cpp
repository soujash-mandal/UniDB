#include "FindPageWithSpaceAction.h"
#include <stdexcept>

FindPageWithSpaceAction::FindPageWithSpaceAction(
    FetchPagePort &fetchPagePort, UnpinPagePort &unpinPagePort,
    WritePagePort &writePagePort, NewPagePort &newPagePort,
    CreateTuplePagePort &createTuplePagePort, uint32_t newTuplePageFreeSpace)
    : fetchPagePort(fetchPagePort), unpinPagePort(unpinPagePort),
      writePagePort(writePagePort), newPagePort(newPagePort),
      newTuplePageFreeSpace(newTuplePageFreeSpace),
      createTuplePagePort(createTuplePagePort) {}

TuplePageId FindPageWithSpaceAction::execute(FSMPageId rootFsmPageId,
                                             uint32_t requiredSpace) {

  FSMPageId fsmPageId = rootFsmPageId;
  while (FSMPage::isValidPage(fsmPageId)) {
    FSMPage fsmPage = fetchPagePort.fetchPage(fsmPageId);
    TuplePageId tuplePageId = fsmPage.findPageWithSpace(requiredSpace);

    // CASE 1: tuple with required space found
    if (FSMPage::isValidPage(tuplePageId)) {
      unpinPagePort.unpinPage(fsmPageId);
      return tuplePageId;
    }

    // CASE 2 : Not found
    else {
      FSMPageId nextFsmPageId = fsmPage.getNextPageId();

      // CASE 2.1 : valid next page go to next page
      if (FSMPage::isValidPage(nextFsmPageId)) {
        unpinPagePort.unpinPage(fsmPageId);
        fsmPageId = nextFsmPageId;
      }

      // CASE 2.2 : Next page not exist
      else {

        // CASE  2.2.1 : Current FSM page have space to create tuple page
        if (!fsmPage.isFull()) {
          TuplePageId tuplePageId = createTuplePagePort.createTuplePage();
          fsmPage.insert(tuplePageId, newTuplePageFreeSpace);
          writePagePort.writePage(fsmPageId, fsmPage);
          unpinPagePort.unpinPage(fsmPageId);
          return tuplePageId;
        }
        // CASE  2.2.2 : Current FSM page is Full
        else {
          nextFsmPageId = newPagePort.newPage();

          FSMPage newFsmPage;
          writePagePort.writePage(nextFsmPageId, newFsmPage);

          fsmPage.setNextPageId(nextFsmPageId);
          writePagePort.writePage(fsmPageId, fsmPage);
          unpinPagePort.unpinPage(fsmPageId);

          fsmPageId = nextFsmPageId;
        }
      }
    }
  }
  throw std::runtime_error("No valid FSM page found");
}
