#pragma once

#include "../domain/RdbMetadataPage.h"

class RdbMetadataUnpinPagePort {
public:
  virtual ~RdbMetadataUnpinPagePort() = default;
  virtual void unpinPage(CatalogPageId pageId) = 0;
};
