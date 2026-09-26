#include "ReadFreePageMetadataAdapter.h"

#include <cstring>

ReadFreePageMetadataAdapter::ReadFreePageMetadataAdapter(
    ReadPageAction &readPageAction)
    : readPageAction(readPageAction) {}

MetadataPage ReadFreePageMetadataAdapter::readPage(MetadataPageId pageId) {
  DiskPage diskPage = readPageAction.execute(pageId);
  MetadataPage freePageMetadataPage;
  std::memcpy(freePageMetadataPage.data(), diskPage.data(), PAGE_SIZE);
  return freePageMetadataPage;
}
