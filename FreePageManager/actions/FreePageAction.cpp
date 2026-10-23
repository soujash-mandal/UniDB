#include "FreePageAction.h"
#include "../domain/MetadataPage.h"

#include <stdexcept>

FreePageAction::FreePageAction(
    ReadFreePageMetadataPort &readMetadataPort,
    WriteFreePageMetadataPort &writeMetadataPort,
    DiscardPagePort &discardPagePort)
    : readMetadataPort(readMetadataPort), writeMetadataPort(writeMetadataPort),
      discardPagePort(discardPagePort) {}

void FreePageAction::execute(uint32_t pageId) {
  if (pageId == 0 || pageId == INVALID_PAGE_ID) {
    throw std::invalid_argument("Cannot free a reserved or invalid page");
  }

  MetadataPageId metadataPageId = MetadataPage::getFirstMetadataPageId();
  while (MetadataPage::isValidPage(metadataPageId)) {
    MetadataPage metadataPage = readMetadataPort.readDiskPage(metadataPageId);
    const uint32_t firstTrackedPageId = metadataPage.getFirstTrackedPageId();
    const uint64_t endTrackedPageId =
        static_cast<uint64_t>(firstTrackedPageId) + MAX_TRACKED_PAGES;

    if (pageId >= firstTrackedPageId && pageId < endTrackedPageId) {
      if (pageId == metadataPageId) {
        throw std::invalid_argument("Cannot free a metadata page");
      }
      // Invalidate the cache first. A pinned page must never be freed.
      discardPagePort.discardPage(pageId);
      metadataPage.setPageFree(pageId);
      writeMetadataPort.writePage(metadataPageId, metadataPage);
      return;
    }

    metadataPageId = metadataPage.getNextMetadataPageId();
  }

  throw std::runtime_error("No metadata page tracks the requested page");
}
