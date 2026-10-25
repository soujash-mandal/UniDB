#pragma once

#include <cstdint>
#include <vector>

namespace RdbService {

struct RdbCreateTupleWithFreeSpaceResult {
  uint32_t slotId;
  uint32_t remainingFreeSpace;
};

class CreateTupleWithFreeSpacePort {
public:
  virtual ~CreateTupleWithFreeSpacePort() = default;

  virtual RdbCreateTupleWithFreeSpaceResult
  createTupleWithFreeSpace(uint32_t tuplePageId, std::vector<char> tupleData) = 0;
};

} // namespace RdbService
