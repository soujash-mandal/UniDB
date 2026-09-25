#include "WritePageAction.h"

WritePageAction::WritePageAction(DiskPort &diskManager)
    : diskManager(diskManager) {}

void WritePageAction::execute(DiskPageId pageId, DiskPage page) {
  diskManager.writePage(pageId, page);
}
