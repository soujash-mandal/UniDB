#pragma once

#include "../../DiskManager/actions/ReadPageAction.h"
#include "../port/ReadPagePort.h"

class ReadPageAdapter : public ReadPagePort {
public:
  explicit ReadPageAdapter(ReadPageAction &readPageAction);
  BufferPoolPage readPage(BufferPoolPageId pageId) override;

private:
  ReadPageAction &readPageAction;
};
