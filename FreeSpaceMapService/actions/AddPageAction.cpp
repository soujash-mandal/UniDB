#include "AddPageAction.h"

#include <stdexcept>

#include "../domain/FreeSpaceMapPage.h"

AddPageAction::AddPageAction(FetchPagePort &fetchPagePort,
                             UnpinPagePort &unpinPagePort,
                             NewPagePort &newPagePort)
    : fetchPagePort(fetchPagePort), unpinPagePort(unpinPagePort),
      newPagePort(newPagePort) {}

void AddPageAction::execute(TableId tableId, PageId freeSpaceMapPageId,
                            PageId dataPageId, uint32_t freeSpace) {
  // 1 :
  Page &page = fetchPagePort.fetchPage(freeSpaceMapPageId);

  FreeSpaceMapPage freeSpaceMapPage;
  freeSpaceMapPage.readFromPage(page);

  // 2 :
  if (freeSpaceMapPage.getTableId() != tableId) {
    unpinPagePort.unpinPage(freeSpaceMapPageId, false);
    throw std::runtime_error(
        "FreeSpaceMapPage does not belong to the specified table");
  }

  // Current FSM page has space.
  if (!freeSpaceMapPage.isFull()) {
    freeSpaceMapPage.addEntry(dataPageId, freeSpace);
    freeSpaceMapPage.writeToPage(page);
    unpinPagePort.unpinPage(freeSpaceMapPageId, true);
    return;
  }

  // Current FSM page is full.
  NewPageResult result = newPagePort.newPage();
  PageId newFreeSpaceMapPageId = result.pageId;
  Page &newPage = result.page;

  // Initialize the new FSM page.
  FreeSpaceMapPage newFreeSpaceMapPage;
  newFreeSpaceMapPage.setTableId(tableId);
  newFreeSpaceMapPage.setNextPageId(FreeSpaceMapPage::INVALID_PAGE_ID);

  newFreeSpaceMapPage.addEntry(dataPageId, freeSpace);
  newFreeSpaceMapPage.writeToPage(newPage);

  // Link old FSM page -> new FSM page.
  freeSpaceMapPage.setNextPageId(newFreeSpaceMapPageId);
  freeSpaceMapPage.writeToPage(page);

  unpinPagePort.unpinPage(freeSpaceMapPageId, true);
  unpinPagePort.unpinPage(newFreeSpaceMapPageId, true);
}
