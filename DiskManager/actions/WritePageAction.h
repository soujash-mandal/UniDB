#pragma once

#include "../domain/DiskPage.h"
#include "../port/DiskPort.h"

class WritePageAction {
public:
  explicit WritePageAction(DiskPort &diskManager);
  void execute(DiskPageId pageId, DiskPage page);

private:
  DiskPort &diskManager;
};
