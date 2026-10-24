#pragma once
#include <cstdint>
class CreateSpaceMapPagePort {
public:
  virtual ~CreateSpaceMapPagePort() = default;
  virtual uint32_t createSpaceMapPage() = 0;
};
