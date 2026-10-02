#pragma once

#include "../domain/CatalogPage.h"

class AllocateTableIdPort {
public:
  virtual ~AllocateTableIdPort() = default;
  virtual CatalogTableId allocateTableId() = 0;
};
