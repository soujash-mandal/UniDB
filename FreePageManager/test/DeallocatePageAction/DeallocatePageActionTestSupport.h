#pragma once

#include "../../actions/DeallocatePageAction.h"

#include <stdexcept>
#include <unordered_map>

class DeallocateMetadataStore final : public ReadFreePageMetadataPort,
                                     public WriteFreePageMetadataPort {
public:
  std::unordered_map<MetadataPageId, MetadataPage> pages;
  bool failWrites = false;
  uint32_t writes = 0;

  MetadataPage readDiskPage(MetadataPageId pageId) override {
    const auto found = pages.find(pageId);
    if (found == pages.end()) {
      throw std::runtime_error("metadata page not found");
    }
    return found->second;
  }

  void writePage(MetadataPageId pageId, MetadataPage page) override {
    ++writes;
    if (failWrites) {
      throw std::runtime_error("metadata write failed");
    }
    pages[pageId] = page;
  }
};
