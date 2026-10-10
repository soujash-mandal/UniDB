#pragma once

#include "../domain/RdbMetadataPage.h"

class RdbMetadataNewPagePort {
public:
  virtual ~RdbMetadataNewPagePort() = default;
  virtual CatalogPageId newPage() = 0;
};
