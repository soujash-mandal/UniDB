#include "AllocatePageAction.h"

#include "../../core/Page.h"
#include <stdexcept>

namespace {
constexpr PageId FREE_PAGE_METADATA_PAGE_ID = 0;
}

AllocatePageAction::AllocatePageAction(
    ReadFreePageMetadataPort &readMetadataPort)
    : readMetadataPort(readMetadataPort) {}

PageId AllocatePageAction::execute() {
  Page metadataPage;
  readMetadataPort.readPage(FREE_PAGE_METADATA_PAGE_ID, metadataPage);
  throw std::runtime_error("Free page metadata layout is not implemented");
}
