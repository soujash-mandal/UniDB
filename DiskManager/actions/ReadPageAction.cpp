#include "ReadPageAction.h"

ReadPageAction::ReadPageAction(DiskPort &diskManager)
    : diskManager(diskManager) {}

void ReadPageAction::execute(PageId pageId, Page &page) {
  diskManager.readPage(pageId, page);
}
