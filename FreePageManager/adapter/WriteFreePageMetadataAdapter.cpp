#include "WriteFreePageMetadataAdapter.h"

WriteFreePageMetadataAdapter::WriteFreePageMetadataAdapter(
    WritePageAction &writePageAction)
    : writePageAction(writePageAction) {}

void WriteFreePageMetadataAdapter::writePage(PageId pageId, const Page &page) {
  writePageAction.execute(pageId, page);
}
