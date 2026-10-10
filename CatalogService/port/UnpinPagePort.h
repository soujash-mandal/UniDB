#pragma once
#include "../domain/CatalogPage.h"
class CatalogUnpinPagePort {
public:
  virtual ~CatalogUnpinPagePort() = default;
  virtual void unpinPage(CatalogPageId pageId) = 0;
};
