#pragma once
#include <cstdint>
#include <vector>
struct RdbFindPageWithSpaceResult {
  uint32_t tuplePageId;
  uint32_t spaceMapPageId;
};
// RDBService-facing contract for table-page discovery and space-map allocation.
class SpaceMapPort {
public:
  virtual ~SpaceMapPort() = default;
  virtual uint32_t createSpaceMapPage() = 0;
  virtual RdbFindPageWithSpaceResult
  findPageWithSpace(uint32_t rootSpaceMapPageId, uint32_t requiredSpace) = 0;
  virtual std::vector<uint32_t> getTablePages(uint32_t rootSpaceMapPageId) = 0;
};