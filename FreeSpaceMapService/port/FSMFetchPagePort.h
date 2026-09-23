#pragma once

#include "../../core/Page.h"
#include "../../core/PageId.h"

class FSMFetchPagePort {
public:
  virtual ~FSMFetchPagePort() = default;

  virtual Page &fetchPage(PageId pageId) = 0;
};
