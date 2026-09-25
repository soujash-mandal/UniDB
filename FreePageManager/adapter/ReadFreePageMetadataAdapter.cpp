#include "ReadFreePageMetadataAdapter.h"

#include <cstring>

ReadFreePageMetadataAdapter::ReadFreePageMetadataAdapter(
    ReadPageAction &readPageAction)
    : readPageAction(readPageAction) {}

FreePageMetadataPage
ReadFreePageMetadataAdapter::readPage(FreePageMetadataPageId pageId) {
  DiskPage diskPage = readPageAction.execute(pageId);
  FreePageMetadataPage freePageMetadataPage;
  std::memcpy(freePageMetadataPage.data(), diskPage.data(),
              FreePageMetadataPage::PAGE_SIZE);
  return freePageMetadataPage;
}
