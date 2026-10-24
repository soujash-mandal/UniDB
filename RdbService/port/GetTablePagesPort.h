#pragma once
#include <cstdint>
#include <vector>
class GetTablePagesPort {
public:
  virtual ~GetTablePagesPort() = default;
  virtual std::vector<uint32_t> getTablePages(uint32_t rootSpaceMapPageId) = 0;
};
