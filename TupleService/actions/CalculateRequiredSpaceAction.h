#pragma once

#include <cstdint>
#include <vector>

class CalculateRequiredSpaceAction {
public:
  uint32_t execute(const std::vector<char> &tupleData) const;
};
