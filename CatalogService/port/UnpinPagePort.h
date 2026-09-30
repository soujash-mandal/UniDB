#pragma once
#include "../domain/CatalogPage.h"
class UnpinPagePort {
public:
  virtual ~UnpinPagePort() = default;
  virtual void unpinPage(CatalogPageId pageId) = 0;
};
