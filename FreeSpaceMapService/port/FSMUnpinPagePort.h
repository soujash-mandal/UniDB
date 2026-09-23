#pragma once

#include "../../core/PageId.h"

class FSMUnpinPagePort {
public:
  virtual ~FSMUnpinPagePort() = default;

  virtual void unpinPage(PageId pageId, bool dirty) = 0;
};
