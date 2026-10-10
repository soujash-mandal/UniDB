#pragma once

#include "../domain/RdbMetadataPage.h"

class RdbMetadataWritePagePort {
public:
  virtual ~RdbMetadataWritePagePort() = default;
  virtual void writePage(CatalogPageId pageId, RdbMetadataPage page) = 0;
};
