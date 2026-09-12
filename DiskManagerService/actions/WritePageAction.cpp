#include "WritePageAction.h"

WritePageAction::WritePageAction(DiskManagerPort &diskManager)
    : diskManager(diskManager) {}

void WritePageAction::execute(PageId pageId, const Page &page) {
  diskManager.writePage(pageId, page);
}
