#pragma once

#include <cstdint>

struct CreateTuplePageResult {
  uint32_t pageId;
  uint32_t freeSpace;
};

class CreateTuplePagePort {
public:
  virtual ~CreateTuplePagePort() = default;

  virtual CreateTuplePageResult createTuplePage() = 0;
};
