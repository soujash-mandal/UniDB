#include "WriteFreePageMetadataAdapter.h"
#include <cstring>

WriteFreePageMetadataAdapter::WriteFreePageMetadataAdapter(
    WritePageAction &writePageAction)
    : writePageAction(writePageAction) {}

void WriteFreePageMetadataAdapter::writePage(FreePageMetadataPageId pageId,
                                             FreePageMetadataPage page) {
  DiskPage diskPage;
  std::memcpy(diskPage.data(), page.data(), FreePageMetadataPage::PAGE_SIZE);
  writePageAction.execute(pageId, diskPage);
}
