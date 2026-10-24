#pragma once
#include <cstdint>
#include <vector>
class CreateTuplePort {
public:
  virtual ~CreateTuplePort() = default;
  virtual uint32_t createTuple(uint32_t tuplePageId,
                               std::vector<char> tupleData) = 0;
};
