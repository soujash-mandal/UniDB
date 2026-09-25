#include "ReadPageAction.h"

ReadPageAction::ReadPageAction(DiskPort &diskManager)
    : diskManager(diskManager) {}

DiskPage ReadPageAction::execute(DiskPageId pageId) {
  return diskManager.readPage(pageId);
}
