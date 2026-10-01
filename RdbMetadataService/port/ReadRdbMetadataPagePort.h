#pragma once

#include "../domain/RdbMetadataPage.h"

class ReadRdbMetadataPagePort {
public:
  virtual ~ReadRdbMetadataPagePort() = default;
  virtual RdbMetadataPage readPage(CatalogPageId pageId) = 0;
};
