#include "UpdateFreeSpaceAction.h"

#include "../domain/FreeSpaceMapPage.h"

UpdateFreeSpaceAction::UpdateFreeSpaceAction(FetchPagePort &fetchPagePort,
                                             UnpinPagePort &unpinPagePort)
    : fetchPagePort(fetchPagePort), unpinPagePort(unpinPagePort) {}

void UpdateFreeSpaceAction::execute(PageId freeSpaceMapPageId,
                                    PageId dataPageId, uint32_t freeSpace) {
  Page &page = fetchPagePort.fetchPage(freeSpaceMapPageId);

  FreeSpaceMapPage freeSpaceMapPage;
  freeSpaceMapPage.readFromPage(page);

  try {
    freeSpaceMapPage.updateEntry(dataPageId, freeSpace);
  } catch (...) {
    unpinPagePort.unpinPage(freeSpaceMapPageId, false);
    throw;
  }

  freeSpaceMapPage.writeToPage(page);

  unpinPagePort.unpinPage(freeSpaceMapPageId, true);
}
