#include "RdbMetadataPage.h"

#include <cstring>

namespace {

struct Header {
  CatalogTableId nextTableId;
  CatalogPageId catalogRootPageId;
};

} // namespace

RdbMetadataPage::RdbMetadataPage() {
  Header header{};
  header.nextTableId = 0;
  header.catalogRootPageId = INVALID_PAGE_ID;
  std::memcpy(bytes, &header, sizeof(Header));
}

CatalogTableId RdbMetadataPage::getNextTableId() {
  Header header;
  std::memcpy(&header, bytes, sizeof(Header));
  return header.nextTableId;
}

void RdbMetadataPage::setNextTableId(CatalogTableId tableId) {
  Header header;
  std::memcpy(&header, bytes, sizeof(Header));
  header.nextTableId = tableId;
  std::memcpy(bytes, &header, sizeof(Header));
}

CatalogPageId RdbMetadataPage::getCatalogRootPageId() {
  Header header;
  std::memcpy(&header, bytes, sizeof(Header));
  return header.catalogRootPageId;
}

void RdbMetadataPage::setCatalogRootPageId(CatalogPageId pageId) {
  Header header;
  std::memcpy(&header, bytes, sizeof(Header));
  header.catalogRootPageId = pageId;
  std::memcpy(bytes, &header, sizeof(Header));
}
