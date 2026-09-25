#pragma once

#include "../domain/BufferPoolPage.h"
#include "../domain/BufferPoolPageId.h"

class WritePagePort {
public:
  virtual ~WritePagePort() = default;
  virtual void writePage(BufferPoolPageId pageId, BufferPoolPage page) = 0;
};
