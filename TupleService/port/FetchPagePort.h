#pragma once

#include "../../core/Page.h"
#include "../../core/PageId.h"

class FetchPagePort {
public:
  virtual ~FetchPagePort() = default;

  virtual Page &fetchPage(PageId pageId) = 0;
};
