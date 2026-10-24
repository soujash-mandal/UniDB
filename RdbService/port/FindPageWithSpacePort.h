#pragma once
#include <cstdint>
struct RdbFindPageWithSpaceResult {
  uint32_t tuplePageId;
  uint32_t spaceMapPageId;
};
class FindPageWithSpacePort {
public:
  virtual ~FindPageWithSpacePort() = default;
  virtual RdbFindPageWithSpaceResult
  findPageWithSpace(uint32_t rootSpaceMapPageId, uint32_t requiredSpace) = 0;
};
