#include "InitializeCatalogAction.h"

#include "../../core/SystemPageIds.h"
#include "../domain/CatalogPage.h"

InitializeCatalogAction::InitializeCatalogAction(
    DiskWritePagePort &diskWritePagePort)
    : diskWritePagePort(diskWritePagePort) {}

void InitializeCatalogAction::execute() {
  CatalogPage page;
  diskWritePagePort.writePage(CATALOG_ROOT_PAGE_ID, page);
}
