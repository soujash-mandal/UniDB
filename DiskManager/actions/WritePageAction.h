#pragma once

#include "../../core/PageId.h"
#include "../port/DiskManagerPort.h"

class WritePageAction {
public:
  explicit WritePageAction(DiskManagerPort &diskManager);
  void execute(PageId pageId, const Page &page);

private:
  DiskManagerPort &diskManager;
};
