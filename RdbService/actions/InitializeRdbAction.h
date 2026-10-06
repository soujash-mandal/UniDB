#pragma once

#include "../port/CreateRdbMetadataPagePort.h"
#include "../port/CreateRootCatalogPagePort.h"
#include "../port/UpdateRdbMetadataPagePort.h"
#include <cstdint>

using MetadataPageId = uint32_t;

class InitializeRdbAction {
public:
  InitializeRdbAction(CreateRdbMetadataPagePort &createRdbMetadataPagePort,
                      CreateRootCatalogPagePort &createRootCatalogPagePort,
                      UpdateRdbMetadataPagePort &updateRdbMetadataPagePort);

  MetadataPageId execute();

private:
  CreateRdbMetadataPagePort &createRdbMetadataPagePort;
  CreateRootCatalogPagePort &createRootCatalogPagePort;
  UpdateRdbMetadataPagePort &updateRdbMetadataPagePort;
};
