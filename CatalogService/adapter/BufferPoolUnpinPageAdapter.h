#pragma once
#include "../../BufferPoolManager/actions/UnpinPageAction.h"
#include "../port/UnpinPagePort.h"

class CatalogBufferPoolUnpinPageAdapter : public CatalogUnpinPagePort {
public:
  explicit CatalogBufferPoolUnpinPageAdapter(UnpinPageAction &unpinPageAction);
  void unpinPage(CatalogPageId pageId) override;

private:
  UnpinPageAction &unpinPageAction;
};
