#pragma once

#include "../../core/PageId.h"
#include "../port/DiskPort.h"

class WritePageAction {
public:
  explicit WritePageAction(DiskPort &diskManager);
  void execute(PageId pageId, const Page &page);

private:
  DiskPort &diskManager;
};
