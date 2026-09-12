#pragma once

#include "../port/DiskManagerPort.h"

class ReadPageAction {
public:
  explicit ReadPageAction(DiskManagerPort &diskManager);
  void execute(PageId pageId, Page &page);
private:
  DiskManagerPort &diskManager;
};