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
      metadataPageId = metadataPage.getNextMetadataPageId();
    } else {
      metadataPage.setPageOccupied(newPageId);
      writeMetadataPort.writePage(metadataPageId, metadataPage);
      return newPageId;
    }
  }
  throw std::runtime_error("No free pages available");
}
