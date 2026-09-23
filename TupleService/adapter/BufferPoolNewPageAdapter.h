#pragma once

#include "../../BufferPoolManager/actions/NewPageAction.h"
#include "../port/NewPagePort.h"

class BufferPoolNewPageAdapter : public NewPagePort {
public:
  explicit BufferPoolNewPageAdapter(NewPageAction &newPageAction);

  NewPageResult newPage() override;

private:
  NewPageAction &newPageAction;
};
