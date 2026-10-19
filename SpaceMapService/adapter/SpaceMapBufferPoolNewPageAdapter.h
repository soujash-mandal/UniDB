#pragma once

#include "../../BufferPoolManager/actions/NewPageAction.h"
#include "../port/NewPagePort.h"

class SpaceMapBufferPoolNewPageAdapter : public NewPagePort {
public:
  explicit SpaceMapBufferPoolNewPageAdapter(NewPageAction &newPageAction);
  SpaceMapPageId newPage() override;

private:
  NewPageAction &newPageAction;
};
