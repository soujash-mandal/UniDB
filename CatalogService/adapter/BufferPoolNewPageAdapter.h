#pragma once
#include "../../BufferPoolManager/actions/NewPageAction.h"
#include "../port/NewPagePort.h"

class CatalogBufferPoolNewPageAdapter : public CatalogNewPagePort {
public:
  explicit CatalogBufferPoolNewPageAdapter(NewPageAction &newPageAction);
  CatalogPageId newPage() override;

private:
  NewPageAction &newPageAction;
};
