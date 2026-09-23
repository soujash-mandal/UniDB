#include "CatalogMetadataPage.h"

#include <cstring>

CatalogMetadataPage::CatalogMetadataPage() : metadata() {}

CatalogMetadata CatalogMetadataPage::getMetadata() const { return metadata; }

void CatalogMetadataPage::setMetadata(const CatalogMetadata &metadata) {
  this->metadata = metadata;
}

void CatalogMetadataPage::readFromPage(const Page &page) {
  std::memcpy(&metadata, page.data(), sizeof(CatalogMetadata));
}

void CatalogMetadataPage::writeToPage(Page &page) const {
  std::memset(page.data(), 0, Page::PAGE_SIZE);
  std::memcpy(page.data(), &metadata, sizeof(CatalogMetadata));
}
