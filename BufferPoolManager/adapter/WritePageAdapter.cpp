#include "WritePageAdapter.h"

WritePageAdapter::WritePageAdapter(WritePageAction &writePageAction)
    : writePageAction(writePageAction) {}

void WritePageAdapter::writePage(PageId pageId, const Page &page) {
  writePageAction.execute(pageId, page);
}
