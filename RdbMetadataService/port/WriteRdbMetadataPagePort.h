#pragma once

#include "../domain/RdbMetadataPage.h"

class WriteRdbMetadataPagePort {
public:
  virtual ~WriteRdbMetadataPagePort() = default;

  virtual void writePage(CatalogPageId pageId, RdbMetadataPage page) = 0;
};
