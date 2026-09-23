#pragma once

#include "Page.h"
#include "PageId.h"

struct NewPageResult {
  PageId pageId;
  Page &page;
};
