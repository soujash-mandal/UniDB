#include "AllocatePageAction.h"
#include "../domain/MetadataPage.h"

#include <stdexcept>

AllocatePageAction::AllocatePageAction(
    ReadFreePageMetadataPort &readMetadataPort,
    WriteFreePageMetadataPort &writeMetadataPort)
    : readMetadataPort(readMetadataPort), writeMetadataPort(writeMetadataPort) {
}

PageId AllocatePageAction::execute() {

  // 1: Find first metadata page
  MetadataPageId metadataPageId = MetadataPage::getFirstMetadataPageId();

  while (MetadataPage::isValidPage(metadataPageId)) {
    // 2. find in this metadata page if a page is available to allocate
    MetadataPage metadataPage = readMetadataPort.readPage(metadataPageId);
    PageId newPageId = metadataPage.findFirstFreePage();

    if (MetadataPage::isValidPage(newPageId)) {
      //  3.1 if we find a free page in that metadata page return that
      metadataPage.setPageOccupied(newPageId);
      writeMetadataPort.writePage(metadataPageId, metadataPage);
      return newPageId;

    } else {
      // 3.2 not found any free page -> go to next
      MetadataPageId nextMetadataPageId = metadataPage.getNextMetadataPageId();

      if (MetadataPage::isValidPage(nextMetadataPageId)) {
        // 4.1 have already assigned next metadatapage
        metadataPageId = nextMetadataPageId;
        continue;

      } else {
        // 4.2 dont have next metadatapage

        // 5. create new metadata page
        MetadataPage newMetadataPage;
        MetadataPageId newMetadataPageId = metadataPageId + 1;
        newMetadataPage.setFirstTrackedPageId(newMetadataPageId);
        newMetadataPage.setPageOccupied(newMetadataPageId);

        // 6. allocate new page
        newPageId = newMetadataPageId + 1;
        newMetadataPage.setPageOccupied(newPageId);
        writeMetadataPort.writePage(newMetadataPageId, newMetadataPage);

        // 7. chain new metadatapage with old one
        metadataPage.setNextMetadataPageId(newMetadataPageId);
        writeMetadataPort.writePage(metadataPageId, metadataPage);

        return newPageId;
      }
    }
  }

  throw std::runtime_error("Failed to allocate page");
}
