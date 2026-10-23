#include "../actions/FreePageAction.h"
#include "../actions/AllocatePageAction.h"

#include <gtest/gtest.h>

#include <stdexcept>

namespace {
class MetadataStore final : public ReadFreePageMetadataPort,
                            public WriteFreePageMetadataPort {
public:
  MetadataPage metadata;
  MetadataPage readDiskPage(MetadataPageId pageId) override {
    EXPECT_EQ(pageId, 0U);
    return metadata;
  }
  void writePage(MetadataPageId pageId, MetadataPage page) override {
    EXPECT_EQ(pageId, 0U);
    metadata = page;
  }
};

class DiscardRecorder final : public DiscardPagePort {
public:
  void discardPage(uint32_t pageId) override { discarded.push_back(pageId); }
  std::vector<uint32_t> discarded;
};
} // namespace

TEST(FreePageActionTest, FreesPageForReuseAfterInvalidatingCachedCopy) {
  MetadataStore metadata;
  DiscardRecorder discard;
  for (uint32_t pageId = 1; pageId <= 10; ++pageId)
    metadata.metadata.setPageOccupied(pageId);

  FreePageAction freePage(metadata, metadata, discard);
  freePage.execute(10);

  EXPECT_EQ(discard.discarded, (std::vector<uint32_t>{10}));
  AllocatePageAction allocatePage(metadata, metadata);
  EXPECT_EQ(allocatePage.execute(), 10U);
}

TEST(FreePageActionTest, RejectsReservedPageWithoutChangingMetadata) {
  MetadataStore metadata;
  DiscardRecorder discard;
  FreePageAction freePage(metadata, metadata, discard);

  EXPECT_THROW(freePage.execute(0), std::invalid_argument);
  EXPECT_TRUE(discard.discarded.empty());
}
