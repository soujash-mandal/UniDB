#include "AllocatePageAction.h"
#include "../domain/FreePageMetadataPage.h"

#include <stdexcept>

namespace {
constexpr FreePageMetadataPageId FIRST_METADATA_PAGE_ID = 0;
}

AllocatePageAction::AllocatePageAction(
    ReadFreePageMetadataPort &readMetadataPort,
    WriteFreePageMetadataPort &writeMetadataPort)
    : readMetadataPort(readMetadataPort), writeMetadataPort(writeMetadataPort) {
}

FreePageMetadataPageId AllocatePageAction::execute() {
  FreePageMetadataPageId metadataPageId = FIRST_METADATA_PAGE_ID;
  while (metadataPageId != FreePageMetadataPage::INVALID_PAGE_ID) {
    FreePageMetadataPage metadataPage =
        readMetadataPort.readPage(metadataPageId);
    FreePageMetadataPageId pageId = metadataPage.findFirstFreePage();
    if (pageId != FreePageMetadataPage::INVALID_PAGE_ID) {
      metadataPage.setPageOccupied(pageId, true);
      writeMetadataPort.writePage(metadataPageId, metadataPage);
      return pageId;
    }
    metadataPageId = metadataPage.getNextMetadataPageId();
  }

  throw std::runtime_error("No free pages available");
}
