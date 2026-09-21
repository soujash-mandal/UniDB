#pragma once

#include "../../EvictionPolicy/EvictionPolicy.h"
#include "../../core/Page.h"
#include "../../core/PageId.h"
#include "../domain/BufferPool.h"
#include "../port/ReadPagePort.h"
#include "../port/WritePagePort.h"

class FetchPageAction {

public:
  FetchPageAction(BufferPool &bufferPool, ReadPagePort &readPagePort,
                  WritePagePort &writePagePort, EvictionPolicy &evictionPolicy);

  Page &execute(PageId pageId);

private:
  BufferPool &bufferPool;
  ReadPagePort &readPagePort;
  WritePagePort &writePagePort;
  EvictionPolicy &evictionPolicy;
};
