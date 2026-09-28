#pragma once

#include "../../BufferPoolManager/actions/WritePageAction.h"
#include "../port/WritePagePort.h"

class BufferPoolWritePageAdapter : public WritePagePort {
public:
  explicit BufferPoolWritePageAdapter(WritePageAction &writePageAction);
  void writePage(PageId pageId, Page page) override;

private:
  WritePageAction &writePageAction;
};
