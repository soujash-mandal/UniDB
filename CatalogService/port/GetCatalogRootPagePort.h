#pragma once

#include "../domain/CatalogPage.h"

class GetCatalogRootPagePort {
public:
  virtual ~GetCatalogRootPagePort() = default;
  virtual CatalogPageId getCatalogRootPageId() = 0;
};
