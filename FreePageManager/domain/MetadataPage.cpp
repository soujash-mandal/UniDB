#include "MetadataPage.h"

#include <stdexcept>

MetadataPage::MetadataPage()
    : nextMetadataPageId(INVALID_PAGE_ID), firstTrackedPageId(0), bitmap{} {
  setPageOccupied(0);
}

void MetadataPage::setPageOccupied(PageId pageId) {
  // This bitmap only tracks pages in this metadata page's range.
  // The allocator must move to the next metadata page when needed.
  if (pageId == INVALID_PAGE_ID || pageId < firstTrackedPageId) {
    throw std::out_of_range("PageId is outside the metadata bitmap range");
  }

  const uint32_t relativePageId = pageId - firstTrackedPageId;
  if (relativePageId >= MAX_TRACKED_PAGES) {
    throw std::out_of_range("PageId is outside the metadata bitmap range");
  }

  // Convert the page's position in this range into a bitmap byte and bit.
  const uint32_t byteIndex = relativePageId / BITS_PER_BYTE;
  const uint32_t bitIndex = relativePageId % BITS_PER_BYTE;
  const uint8_t mask = static_cast<uint8_t>(1U << bitIndex);
  bitmap[byteIndex] |= mask;
}

void MetadataPage::setPageFree(PageId pageId) {
  if (pageId == INVALID_PAGE_ID || pageId < firstTrackedPageId) {
    throw std::out_of_range("PageId is outside the metadata bitmap range");
  }
  const uint32_t relativePageId = pageId - firstTrackedPageId;
  if (relativePageId >= MAX_TRACKED_PAGES) {
    throw std::out_of_range("PageId is outside the metadata bitmap range");
  }
  const uint32_t byteIndex = relativePageId / BITS_PER_BYTE;
  const uint32_t bitIndex = relativePageId % BITS_PER_BYTE;
  const uint8_t mask = static_cast<uint8_t>(1U << bitIndex);
  bitmap[byteIndex] &= static_cast<uint8_t>(~mask);
}

bool MetadataPage::isPageOccupied(PageId pageId) const {
  if (pageId == INVALID_PAGE_ID || pageId < firstTrackedPageId) {
    throw std::out_of_range("PageId is outside the metadata bitmap range");
  }
  const uint32_t relativePageId = pageId - firstTrackedPageId;
  if (relativePageId >= MAX_TRACKED_PAGES) {
    throw std::out_of_range("PageId is outside the metadata bitmap range");
  }
  const uint32_t byteIndex = relativePageId / BITS_PER_BYTE;
  const uint32_t bitIndex = relativePageId % BITS_PER_BYTE;
  const uint8_t mask = static_cast<uint8_t>(1U << bitIndex);
  return (bitmap[byteIndex] & mask) != 0;
}

PageId MetadataPage::findFirstFreePage() const {
  for (uint32_t offset = 0; offset < MAX_TRACKED_PAGES; ++offset) {
    const uint32_t byteIndex = offset / BITS_PER_BYTE;
    const uint32_t bitIndex = offset % BITS_PER_BYTE;
    const uint8_t mask = static_cast<uint8_t>(1U << bitIndex);
    if ((bitmap[byteIndex] & mask) == 0) {
      const PageId pageId = firstTrackedPageId + offset;
      if (pageId == INVALID_PAGE_ID) {
        return INVALID_PAGE_ID;
      }
      return pageId;
    }
  }
  return INVALID_PAGE_ID;
}
