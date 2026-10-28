#pragma once

#include "../domain/BufferPoolPageId.h"

class DeallocatePagePort {
public:
  virtual ~DeallocatePagePort() = default;
  virtual void deallocatePage(BufferPoolPageId pageId) = 0;
};
