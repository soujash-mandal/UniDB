#pragma once

#include "../../core/Page.h"
#include "../../core/PageId.h"
#include "../domain/BufferPool.h"

class NewPageAction {

public:
  explicit NewPageAction(BufferPool &bufferPool);
  Page &execute(PageId pageId);

private:
  BufferPool &bufferPool;
};
