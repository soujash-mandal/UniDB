#pragma once
#include <cstdint>
#include <vector>

struct CreateTupleResult {
  uint32_t slotId;
  uint32_t freeSpace;
};

class CreateTuplePort {
public:
  virtual ~CreateTuplePort() = default;
  virtual CreateTupleResult createTuple(uint32_t tuplePageId,
                                        std::vector<char> tupleData) = 0;
};
