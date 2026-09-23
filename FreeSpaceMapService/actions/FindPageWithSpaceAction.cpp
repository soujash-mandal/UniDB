#include "FindPageWithSpaceAction.h"

#include "../domain/FreeSpaceMapPage.h"

FindPageWithSpaceAction::FindPageWithSpaceAction(
    FSMFetchPagePort &fetchPagePort, FSMUnpinPagePort &unpinPagePort)
    : fetchPagePort(fetchPagePort), unpinPagePort(unpinPagePort) {}

PageId FindPageWithSpaceAction::execute(TableId tableId,
                                        PageId firstFreeSpaceMapPageId,
                                        uint32_t requiredSpace) {
  PageId currentPageId = firstFreeSpaceMapPageId;

  while (currentPageId != FreeSpaceMapPage::INVALID_PAGE_ID) {
    Page &page = fetchPagePort.fetchPage(currentPageId);

    FreeSpaceMapPage freeSpaceMapPage;
    freeSpaceMapPage.readFromPage(page);

    if (freeSpaceMapPage.getTableId() != tableId) {
      unpinPagePort.unpinPage(currentPageId, false);
      return FreeSpaceMapPage::INVALID_PAGE_ID;
    }

    PageId dataPageId = freeSpaceMapPage.findPageWithSpace(requiredSpace);

    PageId nextPageId = freeSpaceMapPage.getNextPageId();

    unpinPagePort.unpinPage(currentPageId, false);

    if (dataPageId != FreeSpaceMapPage::INVALID_PAGE_ID) {
      return dataPageId;
    }

    currentPageId = nextPageId;
  }

  return FreeSpaceMapPage::INVALID_PAGE_ID;
}
