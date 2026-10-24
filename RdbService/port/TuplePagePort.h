#pragma once
#include <cstdint>
#include <vector>
struct RdbCreatedTuplePage {
  uint32_t pageId;
  uint32_t freeSpace;
};
// RDBService-facing contract for tuple-page and row operations.
class TuplePagePort {
public:
  virtual ~TuplePagePort() = default;
  virtual RdbCreatedTuplePage createTuplePage() = 0;
  virtual uint32_t createTuple(uint32_t tuplePageId,
                               std::vector<char> tupleData) = 0;
  virtual std::vector<char> getTuple(uint32_t tuplePageId,
                                     uint32_t tupleSlotId) = 0;
  virtual std::vector<std::vector<char>> getAllTuples(uint32_t tuplePageId) = 0;
};