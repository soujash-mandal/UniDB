#pragma once

#include "../domain/type.h"

#include <cstdint>

class RdbMetadataPort {
public:
  virtual ~RdbMetadataPort() = default;
  virtual CatalogPageId getCatalogRootPageId() = 0;
  virtual uint32_t allocateNextTableId() = 0;
};
