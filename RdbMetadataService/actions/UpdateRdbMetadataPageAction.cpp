#include "UpdateRdbMetadataPageAction.h"

UpdateRdbMetadataPageAction::UpdateRdbMetadataPageAction(
    ReadRdbMetadataPagePort &readPagePort,
    WriteRdbMetadataPagePort &writePagePort)
    : readPagePort(readPagePort), writePagePort(writePagePort) {}

void UpdateRdbMetadataPageAction::execute(CatalogPageId pageId,
                                          CatalogTableId nextTableId,
                                          CatalogPageId catalogRootPageId) {

  RdbMetadataPage page = readPagePort.readPage(pageId);

  page.setNextTableId(nextTableId);
  page.setCatalogRootPageId(catalogRootPageId);

  writePagePort.writePage(pageId, page);
}
