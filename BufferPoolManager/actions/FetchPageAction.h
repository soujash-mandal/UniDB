#pragma once

#include "../../core/Page.h"
#include "../../core/PageId.h"
#include "../domain/BufferPool.h"
#include "../port/ReadPagePort.h"

class FetchPageAction {

public:
  FetchPageAction(BufferPool &bufferPool, ReadPagePort &readPagePort);

  Page &execute(PageId pageId);

private:
  BufferPool &bufferPool;
  ReadPagePort &readPagePort;
};
