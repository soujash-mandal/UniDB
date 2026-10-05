#pragma once

#include "../domain/RdbMetadataPage.h"

class FetchPagePort {
public:
  virtual ~FetchPagePort() = default;
  virtual RdbMetadataPage fetchPage(CatalogPageId pageId) = 0;
};
