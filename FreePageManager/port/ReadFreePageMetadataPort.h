#pragma once

#include "../domain/MetadataPage.h"

class ReadFreePageMetadataPort {
public:
  virtual ~ReadFreePageMetadataPort() = default;
  virtual MetadataPage readDiskPage(MetadataPageId pageId) = 0;
};
