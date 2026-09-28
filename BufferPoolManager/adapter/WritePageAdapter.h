#pragma once

#include "../../DiskManager/actions/WriteDiskPageAction.h"
#include "../port/WritePagePort.h"

class WritePageAdapter : public WritePagePort {
public:
  explicit WritePageAdapter(WriteDiskPageAction &writePageAction);
  void writePage(BufferPoolPageId pageId, BufferPoolPage page) override;

private:
  WriteDiskPageAction &writePageAction;
};
