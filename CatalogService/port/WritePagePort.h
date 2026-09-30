#pragma once
#include "../domain/CatalogPage.h"
class WritePagePort {
public:
  virtual ~WritePagePort() = default;
  virtual void writePage(CatalogPageId pageId, CatalogPage page) = 0;
};
