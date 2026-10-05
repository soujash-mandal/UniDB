#pragma once

#include "../domain/RdbMetadataPage.h"

class NewPagePort {
public:
  virtual ~NewPagePort() = default;
  virtual CatalogPageId newPage() = 0;
};
