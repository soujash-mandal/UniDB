#pragma once

#include "../../RdbMetadataService/domain/RdbMetadataPage.h"

class UpdateRdbMetadataPagePort {
public:
  virtual ~UpdateRdbMetadataPagePort() = default;

  virtual void updateRdbMetadataPage(CatalogPageId pageId,
                                     CatalogPageId catalogRootPageId) = 0;
};
