#pragma once

#include <cstdint>

namespace RdbService {

// Reclaim a newly allocated page whose ownership could not be published.
class ReclaimPagePort {
public:
  virtual ~ReclaimPagePort() = default;
  virtual void reclaimPage(uint32_t pageId) = 0;
};

} // namespace RdbService
