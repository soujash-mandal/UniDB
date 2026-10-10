#pragma once
#include "../domain/CatalogPage.h"
class CatalogWritePagePort {
public:
  virtual ~CatalogWritePagePort() = default;
  virtual void writePage(CatalogPageId pageId, CatalogPage page) = 0;
};
