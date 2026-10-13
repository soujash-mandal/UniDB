#include "MetadataPage.h"

#include <stdexcept>

MetadataPage::MetadataPage()
    : nextMetadataPageId(INVALID_PAGE_ID), firstTrackedPageId(0), bitmap{} {
  setPageOccupied(0);
}

void MetadataPage::setPageOccupied(PageId pageId) {
  // Validate before subtraction or bitmap indexing to prevent out-of-bounds
  // access.
  if (pageId < firstTrackedPageId) {
    throw std::out_of_range(
        "PageId is below the metadata page's tracked range");
  }

  const uint32_t relativePageId = pageId - firstTrackedPageId;
  if (relativePageId >= MAX_TRACKED_PAGES) {
    throw std::out_of_range("PageId is outside the metadata page's bitmap");
  }

  const uint32_t byteIndex = relativePageId / BITS_PER_BYTE;
  const uint32_t bitIndex = relativePageId % BITS_PER_BYTE;
  const uint8_t mask = static_cast<uint8_t>(1U << bitIndex);
  bitmap[byteIndex] |= mask;
}

PageId MetadataPage::findFirstFreePage() const {
  for (uint32_t offset = 0; offset < MAX_TRACKED_PAGES; ++offset) {
    const uint32_t byteIndex = offset / BITS_PER_BYTE;
    const uint32_t bitIndex = offset % BITS_PER_BYTE;
    const uint8_t mask = static_cast<uint8_t>(1U << bitIndex);
    if ((bitmap[byteIndex] & mask) == 0) {
      return firstTrackedPageId + offset;
    }
  }
  return INVALID_PAGE_ID;
}