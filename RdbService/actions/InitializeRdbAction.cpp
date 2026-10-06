#include "InitializeRdbAction.h"

InitializeRdbAction::InitializeRdbAction(
    CreateRdbMetadataPagePort &createRdbMetadataPagePort,
    CreateRootCatalogPagePort &createRootCatalogPagePort,
    UpdateRdbMetadataPagePort &updateRdbMetadataPagePort)
    : createRdbMetadataPagePort(createRdbMetadataPagePort),
      createRootCatalogPagePort(createRootCatalogPagePort),
      updateRdbMetadataPagePort(updateRdbMetadataPagePort) {}

MetadataPageId InitializeRdbAction::execute() {
  MetadataPageId metadataPageId =
      createRdbMetadataPagePort.createRdbMetadataPage();
  CatalogPageId catalogRootPageId =
      createRootCatalogPagePort.createRootCatalogPage();
  updateRdbMetadataPagePort.updateRdbMetadataPage(metadataPageId,
                                                  catalogRootPageId);
  return metadataPageId;
}
