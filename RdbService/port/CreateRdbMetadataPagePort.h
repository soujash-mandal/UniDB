#pragma once
#include "../domain/type.h"

class CreateRdbMetadataPagePort {
public:
  virtual ~CreateRdbMetadataPagePort() = default;
  virtual MetadataPageId createRdbMetadataPage() = 0;
};
