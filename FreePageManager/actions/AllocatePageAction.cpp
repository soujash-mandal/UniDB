#include "AllocatePageAction.h"

#include "../../core/Page.h"
#include <cstdint>
#include <cstring>
#include <stdexcept>

namespace {

constexpr PageId FREE_PAGE_METADATA_PAGE_ID = 0;
constexpr PageId INVALID_PAGE_ID = UINT32_MAX;
constexpr uint32_t BITMAP_OFFSET = sizeof(PageId);

} // namespace

AllocatePageAction::AllocatePageAction(
    ReadFreePageMetadataPort &readMetadataPort,
    WriteFreePageMetadataPort &writeMetadataPort)
    : readMetadataPort(readMetadataPort), writeMetadataPort(writeMetadataPort) {
}

PageId AllocatePageAction::execute() {
  PageId metadataPageId = FREE_PAGE_METADATA_PAGE_ID;
  while (metadataPageId != INVALID_PAGE_ID) {
    Page metadataPage;
    readMetadataPort.readPage(metadataPageId, metadataPage);
    PageId nextMetadataPageId;
    std::memcpy(&nextMetadataPageId, metadataPage.data(), sizeof(PageId));
    uint8_t *bitmap = reinterpret_cast<uint8_t *>(metadataPage.data() + BITMAP_OFFSET);
    constexpr uint32_t bitmapBytes = Page::PAGE_SIZE - BITMAP_OFFSET;
    constexpr uint32_t bitsPerByte = 8;
    for (uint32_t byteIndex = 0; byteIndex < bitmapBytes; ++byteIndex) {
      uint8_t byte = bitmap[byteIndex];
      if (byte == 0xFF) {
        continue;
      }
      for (uint32_t bitIndex = 0; bitIndex < bitsPerByte; ++bitIndex) {
        uint8_t mask = static_cast<uint8_t>(1U << bitIndex);
        if ((byte & mask) == 0) {
          PageId pageId = (metadataPageId * bitmapBytes * bitsPerByte) +
                          (byteIndex * bitsPerByte) + bitIndex;
          if (pageId == FREE_PAGE_METADATA_PAGE_ID) {
            continue;
          }
          bitmap[byteIndex] = static_cast<uint8_t>(byte | mask);
          writeMetadataPort.writePage(metadataPageId, metadataPage);
          return pageId;
        }
      }
    }
    metadataPageId = nextMetadataPageId;
  }

  throw std::runtime_error("No free pages available");
}
