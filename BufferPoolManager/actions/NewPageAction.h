#pragma once

#include "../../EvictionPolicy/EvictionPolicy.h"
#include "../domain/BufferPool.h"
#include "../port/AllocatePagePort.h"
#include "../port/WritePagePort.h"

namespace {
struct NewPageResult {
  BufferPoolPageId pageId;
  BufferPoolPage page;
};
} // namespace

class NewPageAction {
public:
  NewPageAction(BufferPool &bufferPool, AllocatePagePort &allocatePagePort,
                WritePagePort &writePagePort, EvictionPolicy &evictionPolicy);

  NewPageResult execute();

private:
  BufferPool &bufferPool;
  AllocatePagePort &allocatePagePort;
  WritePagePort &writePagePort;
  EvictionPolicy &evictionPolicy;
};