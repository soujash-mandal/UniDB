#include "FreePageMetadata.h"

#include <cstring>

FreePageMetadata::FreePageMetadata()
    : nextMetadataPageId(INVALID_PAGE_ID), firstTrackedPageId(0), bitmap{} {}

PageId FreePageMetadata::getNextMetadataPageId() const {
  return nextMetadataPageId;
}

void FreePageMetadata::setNextMetadataPageId(PageId pageId) {
  nextMetadataPageId = pageId;
}

PageId FreePageMetadata::getFirstTrackedPageId() const {
  return firstTrackedPageId;
}

void FreePageMetadata::setFirstTrackedPageId(PageId pageId) {
  firstTrackedPageId = pageId;
}

bool FreePageMetadata::isPageOccupied(PageId pageId) const {
  if (pageId < firstTrackedPageId) {
    return false;
  }
  PageId relativePageId = pageId - firstTrackedPageId;
  if (relativePageId >= MAX_TRACKED_PAGES) {
    return false;
  }
  uint32_t byteIndex = relativePageId / BITS_PER_BYTE;
  uint32_t bitIndex = relativePageId % BITS_PER_BYTE;
  uint8_t mask = static_cast<uint8_t>(1U << bitIndex);
  return (bitmap[byteIndex] & mask) != 0;
}

void FreePageMetadata::setPageOccupied(PageId pageId, bool occupied) {
  PageId relativePageId = pageId - firstTrackedPageId;
  uint32_t byteIndex = relativePageId / BITS_PER_BYTE;
  uint32_t bitIndex = relativePageId % BITS_PER_BYTE;
  uint8_t mask = static_cast<uint8_t>(1U << bitIndex);
  if (occupied) {
    bitmap[byteIndex] |= mask;
  } else {
    bitmap[byteIndex] &= static_cast<uint8_t>(~mask);
  }
}

PageId FreePageMetadata::findFirstFreePage() const {
  for (uint32_t offset = 0; offset < MAX_TRACKED_PAGES; ++offset) {
    uint32_t byteIndex = offset / BITS_PER_BYTE;
    uint32_t bitIndex = offset % BITS_PER_BYTE;
    uint8_t mask = static_cast<uint8_t>(1U << bitIndex);
    if ((bitmap[byteIndex] & mask) == 0) {
      return firstTrackedPageId + offset;
    }
  }
  return INVALID_PAGE_ID;
}

void FreePageMetadata::readFromPage(const Page &page) {
  std::memcpy(&nextMetadataPageId, page.data(), sizeof(PageId));
  std::memcpy(&firstTrackedPageId, page.data() + FIRST_TRACKED_PAGE_ID_OFFSET,
              sizeof(PageId));
  std::memcpy(bitmap, page.data() + BITMAP_OFFSET, BITMAP_SIZE);
}

void FreePageMetadata::writeToPage(Page &page) const {
  std::memcpy(page.data(), &nextMetadataPageId, sizeof(PageId));
  std::memcpy(page.data() + FIRST_TRACKED_PAGE_ID_OFFSET, &firstTrackedPageId,
              sizeof(PageId));
  std::memcpy(page.data() + BITMAP_OFFSET, bitmap, BITMAP_SIZE);
}
