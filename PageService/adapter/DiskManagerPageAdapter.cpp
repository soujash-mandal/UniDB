#include "DiskManagerPageAdapter.h"

DiskManagerPageAdapter::DiskManagerPageAdapter(ReadPageAction &readPage,
                                               WritePageAction &writePage)
    : readPageAction(readPage), writePageAction(writePage) {}

void DiskManagerPageAdapter::readPage(const PageId &pageId, Page &page) {
  readPageAction.execute(pageId, page);
}

void DiskManagerPageAdapter::writePage(const PageId &pageId, const Page &page) {
  writePageAction.execute(pageId, page);
}