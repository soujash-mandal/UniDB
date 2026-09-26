#include "WriteFreePageMetadataAdapter.h"
#include <cstring>

WriteFreePageMetadataAdapter::WriteFreePageMetadataAdapter(
    WritePageAction &writePageAction)
    : writePageAction(writePageAction) {}

void WriteFreePageMetadataAdapter::writePage(MetadataPageId pageId,
                                             MetadataPage page) {
  DiskPage diskPage;
  std::memcpy(diskPage.data(), page.data(), PAGE_SIZE);
  writePageAction.execute(pageId, diskPage);
}
