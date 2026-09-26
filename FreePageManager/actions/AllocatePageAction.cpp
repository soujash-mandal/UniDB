#include "AllocatePageAction.h"
#include "../domain/MetadataPage.h"

#include <stdexcept>

AllocatePageAction::AllocatePageAction(
    ReadFreePageMetadataPort &readMetadataPort,
    WriteFreePageMetadataPort &writeMetadataPort)
    : readMetadataPort(readMetadataPort), writeMetadataPort(writeMetadataPort) {
}

PageId AllocatePageAction::execute() {
  MetadataPageId metadataPageId = MetadataPage::getFirstMetadataPageId();
  while (MetadataPage::isValidPage(metadataPageId)) {
    MetadataPage metadataPage = readMetadataPort.readPage(metadataPageId);
    PageId newPageId = metadataPage.findFirstFreePage();
    if (!MetadataPage::isValidPage(newPageId)) {
      MetadataPageId nextMetadataPageId = metadataPage.getNextMetadataPageId();
      if (!MetadataPage::isValidPage(nextMetadataPageId)) {
        MetadataPageId newMetadataPageId = metadataPageId + 1;
        metadataPage.setNextMetadataPageId(newMetadataPageId);
        MetadataPage newMetadataPage;
        newMetadataPage.setFirstTrackedPageId(newMetadataPageId);
        writeMetadataPort.writePage(metadataPageId, metadataPage);
        writeMetadataPort.writePage(newMetadataPageId, newMetadataPage);
        newPageId = newMetadataPageId + 1;
        return newPageId;
      }
      metadataPageId = nextMetadataPageId;
    } else {
      metadataPage.setPageOccupied(newPageId);
      writeMetadataPort.writePage(metadataPageId, metadataPage);
      return newPageId;
    }
  }
}
