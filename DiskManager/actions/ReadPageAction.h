#pragma once

#include "../../core/PageId.h"
#include "../port/DiskPort.h"

class ReadPageAction {
public:
  explicit ReadPageAction(DiskPort &diskManager);
  void execute(PageId pageId, Page &page);

private:
  DiskPort &diskManager;
};
