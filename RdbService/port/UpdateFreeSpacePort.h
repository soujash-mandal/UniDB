#pragma once

#include <cstdint>

namespace RdbService {

class UpdateFreeSpacePort {
public:
  virtual ~UpdateFreeSpacePort() = default;

  virtual void updateFreeSpace(uint32_t spaceMapPageId, uint32_t tuplePageId,
                               uint32_t freeSpace) = 0;
};

} // namespace RdbService
