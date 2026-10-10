#pragma once
#include "../domain/CatalogPage.h"
class CatalogFetchPagePort {
public:
  virtual ~CatalogFetchPagePort() = default;
  virtual CatalogPage fetchPage(CatalogPageId pageId) = 0;
};
