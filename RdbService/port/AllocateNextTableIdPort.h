#pragma once

#include <cstdint>

class AllocateNextTableIdPort {
public:
  virtual ~AllocateNextTableIdPort() = default;
  virtual uint32_t allocateNextTableId(uint32_t metadataPageId) = 0;
};
