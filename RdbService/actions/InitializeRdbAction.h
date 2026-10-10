#pragma once

#include "../port/CreateRdbMetadataPagePort.h"
#include "../port/CreateRootCatalogPagePort.h"
#include "../port/UpdateRdbMetadataPagePort.h"
#include <cstdint>

using MetadataPageId = uint32_t;

struct InitializeRdbResult {
  MetadataPageId metadataPageId;
  uint32_t catalogRootPageId;
};

class InitializeRdbAction {
public:
  InitializeRdbAction(CreateRdbMetadataPagePort &createRdbMetadataPagePort,
                      CreateRootCatalogPagePort &createRootCatalogPagePort,
                      UpdateRdbMetadataPagePort &updateRdbMetadataPagePort);

  InitializeRdbResult execute();

private:
  CreateRdbMetadataPagePort &createRdbMetadataPagePort;
  CreateRootCatalogPagePort &createRootCatalogPagePort;
  UpdateRdbMetadataPagePort &updateRdbMetadataPagePort;
};
