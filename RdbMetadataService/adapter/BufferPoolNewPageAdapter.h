#pragma once

#include "../../BufferPoolManager/actions/NewPageAction.h"
#include "../port/NewPagePort.h"

class BufferPoolNewPageAdapter : public RdbMetadataNewPagePort {
public:
  explicit BufferPoolNewPageAdapter(NewPageAction &newPageAction);

  CatalogPageId newPage() override;

private:
  NewPageAction &newPageAction;
};
