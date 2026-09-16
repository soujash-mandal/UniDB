#pragma once

#include "../../DiskManager/actions/WritePageAction.h"
#include "../port/WritePagePort.h"

class WritePageAdapter : public WritePagePort {
public:
  explicit WritePageAdapter(WritePageAction &writePageAction);

  void writePage(PageId pageId, const Page &page) override;

private:
  WritePageAction &writePageAction;
};
