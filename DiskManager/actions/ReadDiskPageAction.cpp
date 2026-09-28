#include "ReadDiskPageAction.h"

ReadDiskPageAction::ReadDiskPageAction(DiskPort &diskManager)
    : diskManager(diskManager) {}

DiskPage ReadDiskPageAction::execute(DiskPageId pageId) {
  return diskManager.readDiskPage(pageId);
}
