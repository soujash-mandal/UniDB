#pragma once

#include "../../core/PageId.h"

class UnpinPagePort {
public:
  virtual ~UnpinPagePort() = default;

  virtual void unpinPage(PageId pageId, bool dirty) = 0;
};
