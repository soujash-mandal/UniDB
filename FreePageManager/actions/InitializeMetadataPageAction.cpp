#include "InitializeMetadataPageAction.h"

#include "../domain/MetadataPage.h"

InitializeMetadataPageAction::InitializeMetadataPageAction(
    WriteFreePageMetadataPort &writeMetadataPort)
    : writeMetadataPort(writeMetadataPort) {}

void InitializeMetadataPageAction::execute() {
  MetadataPage metadataPage;
  writeMetadataPort.writePage(MetadataPage::getFirstMetadataPageId(),
                              metadataPage);
}
