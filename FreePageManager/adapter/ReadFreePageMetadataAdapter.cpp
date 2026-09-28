#include "ReadFreePageMetadataAdapter.h"

#include <cstring>

ReadFreePageMetadataAdapter::ReadFreePageMetadataAdapter(
    ReadDiskPageAction &readDiskPageAction)
    : readDiskPageAction(readDiskPageAction) {}

MetadataPage ReadFreePageMetadataAdapter::readDiskPage(MetadataPageId pageId) {
  DiskPage diskPage = readDiskPageAction.execute(pageId);
  MetadataPage freePageMetadataPage;
  std::memcpy(freePageMetadataPage.data(), diskPage.data(), PAGE_SIZE);
  return freePageMetadataPage;
}
