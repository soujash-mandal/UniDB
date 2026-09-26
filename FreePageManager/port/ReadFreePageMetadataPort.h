#pragma once

#include "../domain/MetadataPage.h"

class ReadFreePageMetadataPort {
public:
  virtual ~ReadFreePageMetadataPort() = default;
  virtual MetadataPage readPage(MetadataPageId pageId) = 0;
};
