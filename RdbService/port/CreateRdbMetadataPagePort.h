#pragma once

#include "../../RdbMetadataService/domain/RdbMetadataPage.h"

class CreateRdbMetadataPagePort {
public:
  virtual ~CreateRdbMetadataPagePort() = default;

  virtual CatalogPageId createRdbMetadataPage() = 0;
};
