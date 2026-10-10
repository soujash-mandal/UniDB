#pragma once

#include "../../BufferPoolManager/actions/UnpinPageAction.h"
#include "../port/UnpinPagePort.h"

class BufferPoolUnpinPageAdapter : public RdbMetadataUnpinPagePort {
public:
  explicit BufferPoolUnpinPageAdapter(UnpinPageAction &unpinPageAction);

  void unpinPage(CatalogPageId pageId) override;

private:
  UnpinPageAction &unpinPageAction;
};
