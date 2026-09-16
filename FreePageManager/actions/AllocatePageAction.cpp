#include "AllocatePageAction.h"
#include "../../core/Page.h"
#include "../domain/FreePageMetadata.h"

#include <stdexcept>

namespace {
constexpr PageId FIRST_METADATA_PAGE_ID = 0;
}

AllocatePageAction::AllocatePageAction(
    ReadFreePageMetadataPort &readMetadataPort,
    WriteFreePageMetadataPort &writeMetadataPort)
    : readMetadataPort(readMetadataPort), writeMetadataPort(writeMetadataPort) {
}

PageId AllocatePageAction::execute() {
  PageId metadataPageId = FIRST_METADATA_PAGE_ID;
  while (metadataPageId != FreePageMetadata::INVALID_PAGE_ID) {
    Page metadataPage;
    readMetadataPort.readPage(metadataPageId, metadataPage);

    FreePageMetadata metadata;
    metadata.readFromPage(metadataPage);

    PageId pageId = metadata.findFirstFreePage();
    if (pageId != FreePageMetadata::INVALID_PAGE_ID) {
      metadata.setPageOccupied(pageId, true);
      metadata.writeToPage(metadataPage);
      writeMetadataPort.writePage(metadataPageId, metadataPage);
      return pageId;
    }
    metadataPageId = metadata.getNextMetadataPageId();
  }

  throw std::runtime_error("No free pages available");
}
