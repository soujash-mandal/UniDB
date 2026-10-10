#pragma once
#include "../domain/CatalogPage.h"
class CatalogNewPagePort {
public:
  virtual ~CatalogNewPagePort() = default;
  virtual CatalogPageId newPage() = 0;
};
