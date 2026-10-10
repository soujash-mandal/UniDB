#include "InitializeRdbAction.h"

InitializeRdbAction::InitializeRdbAction(
    CreateRdbMetadataPagePort &createRdbMetadataPagePort,
    CreateRootCatalogPagePort &createRootCatalogPagePort,
    UpdateRdbMetadataPagePort &updateRdbMetadataPagePort)
    : createRdbMetadataPagePort(createRdbMetadataPagePort),
      createRootCatalogPagePort(createRootCatalogPagePort),
      updateRdbMetadataPagePort(updateRdbMetadataPagePort) {}

MetadataPageId InitializeRdbAction::execute() {
  const MetadataPageId metadataPageId =
      createRdbMetadataPagePort.createRdbMetadataPage();
  const CatalogPageId catalogRootPageId =
      createRootCatalogPagePort.createRootCatalogPage();

  updateRdbMetadataPagePort.updateRdbMetadataPage(metadataPageId,
                                                   catalogRootPageId);
  return metadataPageId;
}
