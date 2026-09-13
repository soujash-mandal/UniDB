#include "DiskManagerPageAdapter.h"

DiskManagerPageAdapter::DiskManagerPageAdapter(ReadPageAction &readPage,
                                               WritePageAction &writePage)
    : readPageAction(readPage), writePageAction(writePage) {}

void DiskManagerPageAdapter::readPage(PageId pageId, Page &page) {
  readPageAction.execute(pageId, page);
}

void DiskManagerPageAdapter::writePage(PageId pageId, const Page &page) {
  writePageAction.execute(pageId, page);
}