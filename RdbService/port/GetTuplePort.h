#pragma once
#include <cstdint>
#include <vector>
class GetTuplePort {
public:
  virtual ~GetTuplePort() = default;
  virtual std::vector<char> getTuple(uint32_t tuplePageId, uint32_t tupleSlotId) = 0;
};
