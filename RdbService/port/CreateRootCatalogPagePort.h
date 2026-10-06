#pragma once
#include "../domain/type.h"
class CreateRootCatalogPagePort {
public:
  virtual ~CreateRootCatalogPagePort() = default;
  virtual CatalogPageId createRootCatalogPage() = 0;
};
