#pragma once

#include "../domain/type.h"
#include "../domain/RdbResult.h"
#include "../port/CreateRdbMetadataPagePort.h"
#include "../port/CreateRootCatalogPagePort.h"
#include "../port/UpdateRdbMetadataPagePort.h"

class InitializeRdbAction {
public:
  InitializeRdbAction(CreateRdbMetadataPagePort &createRdbMetadataPagePort,
                      CreateRootCatalogPagePort &createRootCatalogPagePort,
                      UpdateRdbMetadataPagePort &updateRdbMetadataPagePort);

  unidb::rdb::RdbResult<MetadataPageId> execute();

private:
  CreateRdbMetadataPagePort &createRdbMetadataPagePort;
  CreateRootCatalogPagePort &createRootCatalogPagePort;
  UpdateRdbMetadataPagePort &updateRdbMetadataPagePort;
};
