#include "InitializeRdbAction.h"

InitializeRdbAction::InitializeRdbAction(
    CreateRdbMetadataPagePort &createRdbMetadataPagePort,
    CreateRootCatalogPagePort &createRootCatalogPagePort,
    UpdateRdbMetadataPagePort &updateRdbMetadataPagePort)
    : createRdbMetadataPagePort(createRdbMetadataPagePort),
      createRootCatalogPagePort(createRootCatalogPagePort),
      updateRdbMetadataPagePort(updateRdbMetadataPagePort) {}

InitializeRdbResult InitializeRdbAction::execute() {
  MetadataPageId metadataPageId =
      createRdbMetadataPagePort.createRdbMetadataPage();
  CatalogPageId catalogRootPageId =
      createRootCatalogPagePort.createRootCatalogPage();
  updateRdbMetadataPagePort.updateRdbMetadataPage(metadataPageId,
                                                  catalogRootPageId);
  return {metadataPageId, catalogRootPageId};
}
