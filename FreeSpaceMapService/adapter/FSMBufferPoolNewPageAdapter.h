#pragma once

#include "../../BufferPoolManager/actions/NewPageAction.h"
#include "../port/FSMNewPagePort.h"

class FSMBufferPoolNewPageAdapter : public FSMNewPagePort {
public:
  explicit FSMBufferPoolNewPageAdapter(NewPageAction &newPageAction);

  NewPageResult newPage() override;

private:
  NewPageAction &newPageAction;
};
