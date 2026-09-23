#include "AddPageAction.h"

#include <stdexcept>

#include "../domain/FreeSpaceMapPage.h"

AddPageAction::AddPageAction(FSMFetchPagePort &fetchPagePort,
                             FSMUnpinPagePort &unpinPagePort,
                             FSMNewPagePort &newPagePort)
    : fetchPagePort(fetchPagePort), unpinPagePort(unpinPagePort),
      newPagePort(newPagePort) {}

void AddPageAction::execute(TableId tableId, PageId freeSpaceMapPageId,
                            PageId dataPageId, uint32_t freeSpace) {

  // 1. Fetch and read the FSM page.
  Page &page = fetchPagePort.fetchPage(freeSpaceMapPageId);

  FreeSpaceMapPage freeSpaceMapPage;
  freeSpaceMapPage.readFromPage(page);

  // 2. Validate the table.
  if (freeSpaceMapPage.getTableId() != tableId) {
    unpinPagePort.unpinPage(freeSpaceMapPageId, false);
    throw std::runtime_error(
        "FreeSpaceMapPage does not belong to the specified table");
  }

  // 3. Add to the current FSM page if it has space.
  if (!freeSpaceMapPage.isFull()) {
    freeSpaceMapPage.addEntry(dataPageId, freeSpace);
    freeSpaceMapPage.writeToPage(page);

    unpinPagePort.unpinPage(freeSpaceMapPageId, true);
    return;
  }

  // 4. Create and initialize a new FSM page.
  NewPageResult result = newPagePort.newPage();

  PageId newFreeSpaceMapPageId = result.pageId;
  Page &newPage = result.page;

  FreeSpaceMapPage newFreeSpaceMapPage;
  newFreeSpaceMapPage.setTableId(tableId);
  newFreeSpaceMapPage.setNextPageId(FreeSpaceMapPage::INVALID_PAGE_ID);

  newFreeSpaceMapPage.addEntry(dataPageId, freeSpace);
  newFreeSpaceMapPage.writeToPage(newPage);

  // 5. Link the old FSM page to the new one.
  freeSpaceMapPage.setNextPageId(newFreeSpaceMapPageId);
  freeSpaceMapPage.writeToPage(page);

  unpinPagePort.unpinPage(freeSpaceMapPageId, true);
  unpinPagePort.unpinPage(newFreeSpaceMapPageId, true);
}
