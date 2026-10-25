#pragma once

#include <cstdint>
#include <vector>

namespace RdbService {

class CalculateRequiredSpacePort {
public:
  virtual ~CalculateRequiredSpacePort() = default;

  virtual uint32_t
  calculateRequiredSpace(const std::vector<char> &tupleData) = 0;
};

} // namespace RdbService
