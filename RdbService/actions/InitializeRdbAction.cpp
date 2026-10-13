#include "InitializeRdbAction.h"

InitializeRdbAction::InitializeRdbAction(
    CreateRdbMetadataPagePort &createRdbMetadataPagePort,
    CreateRootCatalogPagePort &createRootCatalogPagePort,
    UpdateRdbMetadataPagePort &updateRdbMetadataPagePort)
    : createRdbMetadataPagePort(createRdbMetadataPagePort),
      createRootCatalogPagePort(createRootCatalogPagePort),
      updateRdbMetadataPagePort(updateRdbMetadataPagePort) {}

RdbResult<MetadataPageId> InitializeRdbAction::execute() {
  try {
    const MetadataPageId metadataPageId = createRdbMetadataPagePort.createRdbMetadataPage();
    const CatalogPageId catalogRootPageId = createRootCatalogPagePort.createRootCatalogPage();
    updateRdbMetadataPagePort.updateRdbMetadataPage(metadataPageId, catalogRootPageId);
    return RdbResult<MetadataPageId>::success(metadataPageId);
  } catch (const std::exception &e) {
    return RdbResult<MetadataPageId>::failure(classifyError(e));
  }
}
