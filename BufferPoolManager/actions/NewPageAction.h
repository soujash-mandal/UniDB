#pragma once

#include "../../EvictionPolicy/EvictionPolicy.h"
#include "../domain/BufferPool.h"
#include "../port/AllocatePagePort.h"
#include "../port/WritePagePort.h"

class NewPageAction {
public:
  struct PageResult {
    BufferPoolPageId pageId;
    BufferPoolPage page;
  };
  NewPageAction(BufferPool &bufferPool, AllocatePagePort &allocatePagePort,
                WritePagePort &writePagePort, EvictionPolicy &evictionPolicy);

  PageResult execute();

private:
  BufferPool &bufferPool;
  AllocatePagePort &allocatePagePort;
  WritePagePort &writePagePort;
  EvictionPolicy &evictionPolicy;
};