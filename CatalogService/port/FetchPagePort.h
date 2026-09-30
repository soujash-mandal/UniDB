#pragma once
#include "../domain/CatalogPage.h"
class FetchPagePort {
public:
  virtual ~FetchPagePort() = default;
  virtual CatalogPage fetchPage(CatalogPageId pageId) = 0;
};
