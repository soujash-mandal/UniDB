#include "AddPageAction.h"

#include <stdexcept>

#include "../domain/FreeSpaceMapPage.h"

AddPageAction::AddPageAction(FetchPagePort &fetchPagePort,
                             UnpinPagePort &unpinPagePort)
    : fetchPagePort(fetchPagePort), unpinPagePort(unpinPagePort) {}

void AddPageAction::execute(TableId tableId, PageId freeSpaceMapPageId,
                            PageId dataPageId, uint32_t freeSpace) {
  Page &page = fetchPagePort.fetchPage(freeSpaceMapPageId);

  FreeSpaceMapPage freeSpaceMapPage;
  freeSpaceMapPage.readFromPage(page);

  if (freeSpaceMapPage.getTableId() != tableId) {
    unpinPagePort.unpinPage(freeSpaceMapPageId, false);
    throw std::runtime_error(
        "FreeSpaceMapPage does not belong to the specified table");
  }

  if (freeSpaceMapPage.isFull()) {
    unpinPagePort.unpinPage(freeSpaceMapPageId, false);
    throw std::runtime_error("FreeSpaceMapPage is full");
  }

  freeSpaceMapPage.addEntry(dataPageId, freeSpace);
  freeSpaceMapPage.writeToPage(page);

  unpinPagePort.unpinPage(freeSpaceMapPageId, true);
}
