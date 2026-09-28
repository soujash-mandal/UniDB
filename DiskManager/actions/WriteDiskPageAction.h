#pragma once

#include "../domain/DiskPage.h"
#include "../port/DiskPort.h"

class WriteDiskPageAction {
public:
  explicit WriteDiskPageAction(DiskPort &diskManager);
  void execute(DiskPageId pageId, DiskPage page);

private:
  DiskPort &diskManager;
};
