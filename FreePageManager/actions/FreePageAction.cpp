#include "FreePageAction.h"

#include "../../core/Page.h"

namespace {
constexpr PageId FREE_PAGE_METADATA_PAGE_ID = 0;
}

FreePageAction::FreePageAction(ReadFreePageMetadataPort &readMetadataPort,
                               WriteFreePageMetadataPort &writeMetadataPort)
    : readMetadataPort(readMetadataPort), writeMetadataPort(writeMetadataPort) {
}

void FreePageAction::execute(PageId pageId) {
  Page metadataPage;
  readMetadataPort.readPage(FREE_PAGE_METADATA_PAGE_ID, metadataPage);
  writeMetadataPort.writePage(FREE_PAGE_METADATA_PAGE_ID, metadataPage);
}
