#include "CatalogPage.h"
#include "../../core/SlotId.h"

#include <cstring>

void CatalogPage::Initialize(Page &page, PageId nextPageId) {
  std::memset(page.data(), 0, Page::PAGE_SIZE);
  auto *header = reinterpret_cast<CatalogPageHeader *>(page.data());
  header->nextPageId = nextPageId;
  header->slotCount = 0;
  header->freeSpaceOffset = Page::PAGE_SIZE;
}

CatalogPageHeader *CatalogPage::GetHeader(Page &page) {
  return reinterpret_cast<CatalogPageHeader *>(page.data());
}

Slot *CatalogPage::GetSlot(Page &page, SlotId slotId) {
  auto *header = GetHeader(page);
  auto *slots =
      reinterpret_cast<Slot *>(page.data() + sizeof(CatalogPageHeader));
  return &slots[slotId];
}

uint16_t CatalogPage::GetFreeSpace(Page &page) {
  auto *header = GetHeader(page);
  uint16_t freeSpaceStart =
      sizeof(CatalogPageHeader) + header->slotCount * sizeof(Slot);
  uint16_t freeSpaceEnd = header->freeSpaceOffset;
  return freeSpaceEnd - freeSpaceStart;
}
