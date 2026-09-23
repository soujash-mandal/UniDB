#pragma once

#include "../../EvictionPolicy/EvictionPolicy.h"
#include "../../core/Page.h"
#include "../../core/PageId.h"
#include "../domain/BufferPool.h"
#include "../port/AllocatePagePort.h"
#include "../port/WritePagePort.h"

struct NewPageResult {
  PageId pageId;
  Page &page;
};

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