#pragma once
#include "../domain/CatalogPage.h"
class NewPagePort {
public:
  virtual ~NewPagePort() = default;
  virtual CatalogPageId newPage() = 0;
};
