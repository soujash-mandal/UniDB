#include "DiskManagerPageAdapter.h"

DiskManagerPageAdapter::DiskManagerPageAdapter(PagePort &pagePort)
    : pagePort(pagePort) {}

void DiskManagerPageAdapter::readPage(PageId pageId, Page &page) {
  pagePort.readPage(pageId, page);
}

void DiskManagerPageAdapter::writePage(PageId pageId, const Page &page) {
  pagePort.writePage(pageId, page);
}