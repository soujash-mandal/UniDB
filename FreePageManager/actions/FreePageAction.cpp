#include "FreePageAction.h"
#include "../../core/Page.h"
#include "../domain/FreePageMetadata.h"

#include <stdexcept>

namespace {
constexpr PageId FIRST_METADATA_PAGE_ID = 0;
}

FreePageAction::FreePageAction(ReadFreePageMetadataPort &readMetadataPort,
                               WriteFreePageMetadataPort &writeMetadataPort)
    : readMetadataPort(readMetadataPort), writeMetadataPort(writeMetadataPort) {
}

void FreePageAction::execute(PageId pageId) {
  PageId metadataPageId = FIRST_METADATA_PAGE_ID;
  while (metadataPageId != FreePageMetadata::INVALID_PAGE_ID) {
    Page metadataPage;
    readMetadataPort.readPage(metadataPageId, metadataPage);

    FreePageMetadata metadata;
    metadata.readFromPage(metadataPage);

    PageId firstTrackedPageId = metadata.getFirstTrackedPageId();
    PageId lastTrackedPageId =
        firstTrackedPageId + FreePageMetadata::MAX_TRACKED_PAGES;

    if (pageId >= firstTrackedPageId && pageId < lastTrackedPageId) {
      metadata.setPageOccupied(pageId, false);
      metadata.writeToPage(metadataPage);
      writeMetadataPort.writePage(metadataPageId, metadataPage);
      return;
    }
    metadataPageId = metadata.getNextMetadataPageId();
  }
  throw std::runtime_error("Page is not tracked by FreePageManager");
}
