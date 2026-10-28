#include "DeallocatePageAction.h"

#include "../domain/MetadataPage.h"

#include <limits>
#include <stdexcept>

DeallocatePageAction::DeallocatePageAction(
    ReadFreePageMetadataPort &readMetadataPort,
    WriteFreePageMetadataPort &writeMetadataPort)
    : readMetadataPort(readMetadataPort), writeMetadataPort(writeMetadataPort) {}

void DeallocatePageAction::execute(uint32_t pageId) {
  // Step 1: Reject reserved and invalid page IDs before touching metadata.
  if (pageId == MetadataPage::getFirstMetadataPageId() ||
      pageId == std::numeric_limits<uint32_t>::max()) {
    throw std::invalid_argument("Cannot deallocate a reserved or invalid page");
  }

  // Step 2: Find the metadata page whose bitmap owns this page ID.
  uint32_t metadataPageId = MetadataPage::getFirstMetadataPageId();
  while (MetadataPage::isValidPage(metadataPageId)) {
    MetadataPage metadataPage = readMetadataPort.readDiskPage(metadataPageId);
    const uint32_t nextMetadataPageId = metadataPage.getNextMetadataPageId();

    // Step 2.1: Metadata pages themselves must never be returned to the pool.
    if (pageId == metadataPageId || pageId == nextMetadataPageId) {
      throw std::invalid_argument("Cannot deallocate a metadata page");
    }

    const uint32_t firstTrackedPageId = metadataPage.getFirstTrackedPageId();
    const uint64_t endTrackedPageId =
        static_cast<uint64_t>(firstTrackedPageId) + MAX_TRACKED_PAGES;
    if (pageId >= firstTrackedPageId &&
        static_cast<uint64_t>(pageId) < endTrackedPageId) {
      // Step 2.2: Reject double-free and only persist a real allocation change.
      if (!metadataPage.isPageOccupied(pageId)) {
        throw std::invalid_argument("Page is already free");
      }
      metadataPage.setPageFree(pageId);
      writeMetadataPort.writePage(metadataPageId, metadataPage);
      return;
    }

    // Step 2.3: Continue through the metadata chain when this bitmap does not
    // own the requested page ID.
    metadataPageId = nextMetadataPageId;
  }

  throw std::out_of_range("Page is not tracked by the free-page metadata");
}
