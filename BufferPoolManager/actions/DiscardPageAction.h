#pragma once

#include "../../EvictionPolicy/EvictionPolicy.h"
#include "../domain/BufferPool.h"

class DiscardPageAction {
public:
  DiscardPageAction(BufferPool &bufferPool, EvictionPolicy &evictionPolicy);
  void execute(BufferPoolPageId pageId);

private:
  BufferPool &bufferPool;
  EvictionPolicy &evictionPolicy;
};
