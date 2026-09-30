#include "CPDiskWritePageAdapter.h"

#include "../../DiskManager/domain/DiskPage.h"
#include <cstring>

CPDiskWritePageAdapter::CPDiskWritePageAdapter(
    WriteDiskPageAction &writePageAction)
    : writePageAction(writePageAction) {}

void CPDiskWritePageAdapter::writePage(CatalogPageId pageId, CatalogPage page) {
  DiskPage diskPage;
  std::memcpy(diskPage.data(), page.data(), CatalogPage::PAGE_SIZE);
  writePageAction.execute(pageId, diskPage);
}
