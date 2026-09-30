#include "InitializeMetadataPageAction.h"
#include "../../core/SystemPageIds.h"
#include "../domain/MetadataPage.h"

InitializeMetadataPageAction::InitializeMetadataPageAction(
    WriteFreePageMetadataPort &writeMetadataPort)
    : writeMetadataPort(writeMetadataPort) {}

void InitializeMetadataPageAction::execute() {
  MetadataPage metadataPage;
  writeMetadataPort.writePage(METADATA_ROOT_PAGE_ID, metadataPage);
}
