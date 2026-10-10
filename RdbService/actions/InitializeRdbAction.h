#pragma once

#include "../domain/type.h"
#include "../port/CreateRdbMetadataPagePort.h"
#include "../port/CreateRootCatalogPagePort.h"
#include "../port/UpdateRdbMetadataPagePort.h"

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
