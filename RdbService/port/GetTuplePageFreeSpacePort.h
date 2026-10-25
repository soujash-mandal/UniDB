#pragma once

#include <cstdint>

namespace RdbService {

class GetTuplePageFreeSpacePort {
public:
  virtual ~GetTuplePageFreeSpacePort() = default;

  virtual uint32_t getFreeSpace(uint32_t tuplePageId) = 0;
};

} // namespace RdbService
