#pragma once

#include <cstdint>

namespace {
using MetadataPageId = uint32_t;
using PageId = uint32_t;
using NextMetadataPageId = MetadataPageId;
using FirstTrackedPageId = uint32_t;
constexpr uint32_t PAGE_SIZE = 8192;
constexpr uint32_t BITMAP_OFFSET =
    sizeof(NextMetadataPageId) + sizeof(FirstTrackedPageId);
constexpr uint32_t BITMAP_SIZE = PAGE_SIZE - BITMAP_OFFSET;
constexpr uint32_t INVALID_PAGE_ID = UINT32_MAX;
constexpr MetadataPageId FIRST_METADATA_PAGE_ID = 0;
} // namespace

class MetadataPage {
public:
  MetadataPage();
  char *data() { return reinterpret_cast<char *>(this); }
  const char *data() const { return reinterpret_cast<const char *>(this); }

  static bool isValidPage(uint32_t pageId) {
    return !(pageId == INVALID_PAGE_ID);
  }
  static MetadataPageId getFirstMetadataPageId() {
    return FIRST_METADATA_PAGE_ID;
  }

  uint32_t findFirstFreePage() const;

  void setPageOccupied(PageId pageId);

  uint32_t getNextMetadataPageId() { return nextMetadataPageId; };
  void setNextMetadataPageId(uint32_t pageId) { nextMetadataPageId = pageId; };

  uint32_t getFirstTrackedPageId() { return firstTrackedPageId; };
  void setFirstTrackedPageId(uint32_t pageId) { firstTrackedPageId = pageId; };

private:
  uint32_t nextMetadataPageId;
  uint32_t firstTrackedPageId;
  uint8_t bitmap[BITMAP_SIZE];
};
