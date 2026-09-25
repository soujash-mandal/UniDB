// todo: remove all core imports
#pragma once

#include "../../core/Page.h"
#include "../../core/PageId.h"
#include "../../core/SlotId.h"

#include <cstdint>

class TuplePage {
public:
  explicit TuplePage(Page &page);
  static void initialize(Page &page, PageId pageId);

  SlotId insert(const char *tupleData, uint16_t tupleSize);
  void get(SlotId slotId, char *tupleData) const;
  void remove(SlotId slotId);
  bool getAvailableSpace() const;

private:
  Page &page;
};
