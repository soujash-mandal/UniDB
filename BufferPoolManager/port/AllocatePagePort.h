#pragma once

#include "../../core/PageId.h"

class AllocatePagePort {
public:
  virtual ~AllocatePagePort() = default;

  virtual PageId allocatePage() = 0;
};
