#pragma once

#include "../domain/RdbMetadataPage.h"

class RdbMetadataFetchPagePort {
public:
  virtual ~RdbMetadataFetchPagePort() = default;
  virtual RdbMetadataPage fetchPage(CatalogPageId pageId) = 0;
};
