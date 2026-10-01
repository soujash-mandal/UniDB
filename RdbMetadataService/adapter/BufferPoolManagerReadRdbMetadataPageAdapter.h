#pragma once

#include "../../BufferPoolManager/actions/FetchPageAction.h"
#include "../port/ReadRdbMetadataPagePort.h"

class BufferPoolManagerReadRdbMetadataPageAdapter
    : public ReadRdbMetadataPagePort {
public:
  explicit BufferPoolManagerReadRdbMetadataPageAdapter(
      FetchPageAction &fetchPageAction);

  RdbMetadataPage readPage(CatalogPageId pageId) override;

private:
  FetchPageAction &fetchPageAction;
};
