#include "WriteDiskPageAction.h"

WriteDiskPageAction::WriteDiskPageAction(DiskPort &diskManager)
    : diskManager(diskManager) {}

void WriteDiskPageAction::execute(DiskPageId pageId, DiskPage page) {
  diskManager.writePage(pageId, page);
}
