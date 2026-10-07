#include "CreateFpmPageAction.h"
#include "../domain/MetadataPage.h"

CreateFpmPageAction::CreateFpmPageAction(
    WriteFreePageMetadataPort &writeMetadataPort)
    : writeMetadataPort(writeMetadataPort) {}

void CreateFpmPageAction::execute(FpmPageId pageId) {
  MetadataPage metadataPage;
  writeMetadataPort.writePage(pageId, metadataPage);
}
