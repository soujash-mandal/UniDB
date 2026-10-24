#pragma once
#include <cstdint>
struct RdbCreatedTuplePage {
  uint32_t pageId;
  uint32_t freeSpace;
};
class CreateTuplePagePort {
public:
  virtual ~CreateTuplePagePort() = default;
  virtual RdbCreatedTuplePage createTuplePage() = 0;
};
