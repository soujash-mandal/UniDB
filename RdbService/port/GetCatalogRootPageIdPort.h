#pragma once

#include <cstdint>

class GetCatalogRootPageIdPort {
public:
  virtual ~GetCatalogRootPageIdPort() = default;
  virtual uint32_t getCatalogRootPageId(uint32_t metadataPageId) = 0;
};
