#pragma once

#include "../domain/RdbMetadataPage.h"

class WritePagePort {
public:
  virtual ~WritePagePort() = default;
  virtual void writePage(CatalogPageId pageId, RdbMetadataPage page) = 0;
};
