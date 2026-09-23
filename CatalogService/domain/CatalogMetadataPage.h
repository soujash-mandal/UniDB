#pragma once

#include "../../core/Page.h"
#include "CatalogMetadata.h"

class CatalogMetadataPage {
public:
  CatalogMetadataPage();

  CatalogMetadata getMetadata() const;
  void setMetadata(const CatalogMetadata &metadata);

  void readFromPage(const Page &page);
  void writeToPage(Page &page) const;

private:
  CatalogMetadata metadata;
};
