#include "InitializeRdbAction.h"

InitializeRdbAction::InitializeRdbAction(
    CreateRdbMetadataPagePort &createRdbMetadataPagePort,
    CreateRootCatalogPagePort &createRootCatalogPagePort,
    UpdateRdbMetadataPagePort &updateRdbMetadataPagePort)
    : createRdbMetadataPagePort(createRdbMetadataPagePort),
      createRootCatalogPagePort(createRootCatalogPagePort),
      updateRdbMetadataPagePort(updateRdbMetadataPagePort) {}

unidb::rdb::RdbResult<MetadataPageId> InitializeRdbAction::execute() {
  try {
    const MetadataPageId metadataPageId = createRdbMetadataPagePort.createRdbMetadataPage();
    const CatalogPageId catalogRootPageId = createRootCatalogPagePort.createRootCatalogPage();
    updateRdbMetadataPagePort.updateRdbMetadataPage(metadataPageId, catalogRootPageId);
    return unidb::rdb::RdbResult<MetadataPageId>::success(metadataPageId);
  } catch (const std::exception &e) {
    return unidb::rdb::RdbResult<MetadataPageId>::failure(classifyError(e));
  }
}
