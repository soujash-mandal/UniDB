#include "UpdateRdbMetadataPageAction.h"

UpdateRdbMetadataPageAction::UpdateRdbMetadataPageAction(
    RdbMetadataFetchPagePort &fetchPagePort, RdbMetadataWritePagePort &writePagePort,
    RdbMetadataUnpinPagePort &unpinPagePort)
    : readPagePort(fetchPagePort), writePagePort(writePagePort),
      unpinPagePort(unpinPagePort) {}

void UpdateRdbMetadataPageAction::execute(CatalogPageId pageId,
                                          CatalogPageId catalogRootPageId) {
  RdbMetadataPage page = readPagePort.fetchPage(pageId);
  page.setCatalogRootPageId(catalogRootPageId);
  writePagePort.writePage(pageId, page);
  unpinPagePort.unpinPage(pageId);
}
