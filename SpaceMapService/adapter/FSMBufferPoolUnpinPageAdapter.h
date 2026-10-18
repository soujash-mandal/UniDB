#pragma once

#include "../../BufferPoolManager/actions/UnpinPageAction.h"
#include "../port/UnpinPagePort.h"

class FSMBufferPoolUnpinPageAdapter : public UnpinPagePort {
public:
  explicit FSMBufferPoolUnpinPageAdapter(UnpinPageAction &unpinPageAction);
  void unpinPage(FSMPageId pageId) override;

private:
  UnpinPageAction &unpinPageAction;
};
