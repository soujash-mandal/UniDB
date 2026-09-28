// todo: will deprecate this later while optimizing
#pragma once

#include "../domain/BufferPool.h"
#include "../domain/BufferPoolPageId.h"

class WritePageAction {
public:
  explicit WritePageAction(BufferPool &bufferPool);

  void execute(BufferPoolPageId pageId, BufferPoolPage page);

private:
  BufferPool &bufferPool;
};
