#pragma once

#include "../../BufferPoolManager/actions/UnpinPageAction.h"
#include "../port/UnpinPagePort.h"

class SpaceMapBufferPoolUnpinPageAdapter : public UnpinPagePort {
public:
  explicit SpaceMapBufferPoolUnpinPageAdapter(UnpinPageAction &unpinPageAction);
  void unpinPage(SpaceMapPageId pageId) override;

private:
  UnpinPageAction &unpinPageAction;
};
