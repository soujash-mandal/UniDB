#include "ReadPageAdapter.h"

ReadPageAdapter::ReadPageAdapter(ReadPageAction &readPageAction)
    : readPageAction(readPageAction) {}

void ReadPageAdapter::readPage(PageId pageId, Page &page) {
  readPageAction.execute(pageId, page);
}
