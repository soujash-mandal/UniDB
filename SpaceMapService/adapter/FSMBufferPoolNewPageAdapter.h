#pragma once

#include "../../BufferPoolManager/actions/NewPageAction.h"
#include "../port/NewPagePort.h"

class FSMBufferPoolNewPageAdapter : public NewPagePort {
public:
  explicit FSMBufferPoolNewPageAdapter(NewPageAction &newPageAction);
  FSMPageId newPage() override;

private:
  NewPageAction &newPageAction;
};
