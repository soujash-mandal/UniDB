#pragma once

#include "PageId.h"
#include "SlotId.h"

struct TupleId {
  PageId pageId;
  SlotId slotId;

  TupleId(PageId pageId, SlotId slotId) : pageId(pageId), slotId(slotId) {}
};
