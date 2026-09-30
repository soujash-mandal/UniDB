#pragma once

#include "../domain/CatalogPage.h"

class DiskWritePagePort {
public:
  virtual ~DiskWritePagePort() = default;
  virtual void writePage(CatalogPageId pageId, CatalogPage page) = 0;
};
