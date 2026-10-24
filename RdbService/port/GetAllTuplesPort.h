#pragma once
#include <cstdint>
#include <vector>
class GetAllTuplesPort {
public:
  virtual ~GetAllTuplesPort() = default;
  virtual std::vector<std::vector<char>> getAllTuples(uint32_t tuplePageId) = 0;
};
