#include "ReadFreePageMetadataAdapter.h"

ReadFreePageMetadataAdapter::ReadFreePageMetadataAdapter(
    ReadPageAction &readPageAction)
    : readPageAction(readPageAction) {}

void ReadFreePageMetadataAdapter::readPage(PageId pageId, Page &page) {
  readPageAction.execute(pageId, page);
}
