#pragma once

#include "../../core/PageId.h"
#include "../domain/BufferPool.h"
#include "../port/WritePagePort.h"

class FlushPageAction {

public:
  FlushPageAction(BufferPool &bufferPool, WritePagePort &writePagePort);
  void execute(PageId pageId);

private:
  BufferPool &bufferPool;
  WritePagePort &writePagePort;
};
