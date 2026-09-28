#pragma once

#include "../../DiskManager/actions/ReadDiskPageAction.h"
#include "../port/ReadPagePort.h"

class ReadPageAdapter : public ReadPagePort {
public:
  explicit ReadPageAdapter(ReadDiskPageAction &readDiskPageAction);
  BufferPoolPage readDiskPage(BufferPoolPageId pageId) override;

private:
  ReadDiskPageAction &readDiskPageAction;
};
