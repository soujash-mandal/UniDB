#pragma once
#include "../domain/type.h"

class InitializeRdbPort {
public:
  virtual ~InitializeRdbPort() = default;
  virtual RdbMetadataPageId initialize() = 0;
};
