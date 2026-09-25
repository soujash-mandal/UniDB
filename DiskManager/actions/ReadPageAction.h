#pragma once

#include "../port/DiskPort.h"

class ReadPageAction {
public:
  explicit ReadPageAction(DiskPort &diskManager);
  DiskPage execute(DiskPageId pageId);

private:
  DiskPort &diskManager;
};
