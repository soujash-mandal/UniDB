#include "InitializeMetadataPageAction.h"

#include "../domain/MetadataPage.h"

InitializeMetadataPageAction::InitializeMetadataPageAction(
    WriteFreePageMetadataPort &writeMetadataPort)
    : writeMetadataPort(writeMetadataPort) {}

void InitializeMetadataPageAction::execute() {
  MetadataPage metadataPage;
  metadataPage.setFirstTrackedPageId(0);
  metadataPage.setPageOccupied(0);
  writeMetadataPort.writePage(0, metadataPage);
}
