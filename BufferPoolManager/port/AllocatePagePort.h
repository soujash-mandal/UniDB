#pragma once

#include "../domain/BufferPoolPageId.h"

class AllocatePagePort {
public:
  virtual ~AllocatePagePort() = default;
  virtual BufferPoolPageId allocatePage() = 0;
};
