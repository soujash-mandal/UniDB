#pragma once
#include "../domain/type.h"

class UpdateRdbMetadataPagePort {
public:
  virtual ~UpdateRdbMetadataPagePort() = default;

  virtual void updateRdbMetadataPage(MetadataPageId pageId,
                                     CatalogPageId catalogRootPageId) = 0;
};
