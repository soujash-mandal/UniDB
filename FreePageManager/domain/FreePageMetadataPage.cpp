#include "FreePageMetadataPage.h"

#include <cstring>

FreePageMetadataPage::FreePageMetadataPage()
    : nextMetadataPageId(INVALID_PAGE_ID), firstTrackedPageId(0), bitmap{} {
  setPageOccupied(0, true);
}

FreePageMetadataPageId FreePageMetadataPage::getNextMetadataPageId() const {
  return nextMetadataPageId;
}

void FreePageMetadataPage::setNextMetadataPageId(
    FreePageMetadataPageId pageId) {
  nextMetadataPageId = pageId;
}

FreePageMetadataPageId FreePageMetadataPage::getFirstTrackedPageId() const {
  return firstTrackedPageId;
}

void FreePageMetadataPage::setFirstTrackedPageId(
    FreePageMetadataPageId pageId) {
  firstTrackedPageId = pageId;
}

bool FreePageMetadataPage::isPageOccupied(FreePageMetadataPageId pageId) const {
  if (pageId < firstTrackedPageId) {
    return false;
  }

  FreePageMetadataPageId relativePageId = pageId - firstTrackedPageId;

  if (relativePageId >= MAX_TRACKED_PAGES) {
    return false;
  }

  uint32_t byteIndex = relativePageId / BITS_PER_BYTE;
  uint32_t bitIndex = relativePageId % BITS_PER_BYTE;

  uint8_t mask = static_cast<uint8_t>(1U << bitIndex);

  return (bitmap[byteIndex] & mask) != 0;
}

void FreePageMetadataPage::setPageOccupied(FreePageMetadataPageId pageId,
                                           bool occupied) {
  FreePageMetadataPageId relativePageId = pageId - firstTrackedPageId;

  uint32_t byteIndex = relativePageId / BITS_PER_BYTE;
  uint32_t bitIndex = relativePageId % BITS_PER_BYTE;

  uint8_t mask = static_cast<uint8_t>(1U << bitIndex);

  if (occupied) {
    bitmap[byteIndex] |= mask;
  } else {
    bitmap[byteIndex] &= static_cast<uint8_t>(~mask);
  }
}

FreePageMetadataPageId FreePageMetadataPage::findFirstFreePage() const {
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