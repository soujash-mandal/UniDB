#pragma once

#include "../domain/BufferPool.h"
#include "../../EvictionPolicy/EvictionPolicy.h"

class DiscardPageAction {
public:
  DiscardPageAction(BufferPool &bufferPool, EvictionPolicy &evictionPolicy);
  void execute(BufferPoolPageId pageId);

private:
  BufferPool &bufferPool;
  EvictionPolicy &evictionPolicy;
};
