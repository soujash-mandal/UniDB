#pragma once

#include "../port/DiskPort.h"

class ReadDiskPageAction {
public:
  explicit ReadDiskPageAction(DiskPort &diskManager);
  DiskPage execute(DiskPageId pageId);

private:
  DiskPort &diskManager;
};
