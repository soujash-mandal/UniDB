#pragma once

#include "../../BufferPoolManager/actions/WritePageAction.h"
#include "../port/WritePagePort.h"

class SpaceMapBufferPoolWritePageAdapter : public SpaceMapWritePagePort {
public:
  explicit SpaceMapBufferPoolWritePageAdapter(WritePageAction &writePageAction);
  void writePage(SpaceMapPageId pageId, SpaceMapPage page) override;

private:
  WritePageAction &writePageAction;
};
