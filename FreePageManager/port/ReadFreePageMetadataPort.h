#pragma once

#include "../domain/FreePageMetadataPage.h"
#include "../domain/FreePageMetadataPageId.h"

class ReadFreePageMetadataPort {
public:
  virtual ~ReadFreePageMetadataPort() = default;
  virtual FreePageMetadataPage readPage(FreePageMetadataPageId pageId) = 0;
};
