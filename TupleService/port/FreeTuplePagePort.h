#pragma once

#include <cstdint>

class FreeTuplePagePort {
public:
  virtual ~FreeTuplePagePort() = default;
  virtual void freePage(uint32_t pageId) = 0;
};
