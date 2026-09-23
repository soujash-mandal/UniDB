#pragma once

#include "../../BufferPoolManager/actions/UnpinPageAction.h"
#include "../port/FSMUnpinPagePort.h"

class FSMBufferPoolUnpinPageAdapter : public FSMUnpinPagePort {
public:
  explicit FSMBufferPoolUnpinPageAdapter(UnpinPageAction &unpinPageAction);

  void unpinPage(PageId pageId, bool dirty) override;

private:
  UnpinPageAction &unpinPageAction;
};
