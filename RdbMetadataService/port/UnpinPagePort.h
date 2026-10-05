#pragma once

#include "../domain/RdbMetadataPage.h"

class UnpinPagePort {
public:
  virtual ~UnpinPagePort() = default;
  virtual void unpinPage(CatalogPageId pageId) = 0;
};
