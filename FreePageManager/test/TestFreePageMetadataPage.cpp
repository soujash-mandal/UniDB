#include "../actions/AllocatePageAction.h"
#include "../domain/MetadataPage.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <unordered_map>

class MetadataMemoryStore : public ReadFreePageMetadataPort,
                            public WriteFreePageMetadataPort {
public:
  std::unordered_map<MetadataPageId, MetadataPage> pages;
  bool failWrites = false;
  int writeCount = 0;

  MetadataPage readDiskPage(MetadataPageId pageId) override {
    auto it = pages.find(pageId);
    if (it == pages.end()) {
      throw std::runtime_error("metadata page not found");
    }
    return it->second;
  }

  void writePage(MetadataPageId pageId, MetadataPage page) override {
    ++writeCount;
    if (failWrites) {
      throw std::runtime_error("simulated metadata write failure");
    }
    pages[pageId] = page;
  }
};

// Scenario 1: A newly initialized metadata page reserves page 0 and returns
// page 1 as the first free page.
TEST(MetadataPageTest, ReservesMetadataPageAndFindsFirstFreePage) {
  MetadataPage page;

  EXPECT_EQ(page.getFirstTrackedPageId(), 0);
  EXPECT_EQ(page.findFirstFreePage(), 1);
}

// Scenario 2: Marking the current first free page occupied advances
// allocation to the next free PageId.
TEST(MetadataPageTest, MarkingPageOccupiedAdvancesFirstFreePage) {
  MetadataPage page;

  page.setPageOccupied(1);

  EXPECT_EQ(page.findFirstFreePage(), 2);
}

// Scenario 3: Bitmap tracking supports the last valid bit and reports
// exhaustion after every tracked bit is occupied.
TEST(MetadataPageTest, SupportsLastBitmapBitAndDetectsExhaustion) {
  MetadataPage page;

  for (uint32_t pageId = 1; pageId < MAX_TRACKED_PAGES - 1; ++pageId) {
    page.setPageOccupied(pageId);
  }

  const uint32_t lastTrackedPageId = MAX_TRACKED_PAGES - 1;
  EXPECT_EQ(page.findFirstFreePage(), lastTrackedPageId);
  page.setPageOccupied(lastTrackedPageId);

  EXPECT_EQ(page.findFirstFreePage(), INVALID_PAGE_ID);
}

// Scenario 4: A PageId below firstTrackedPageId is rejected instead of
// underflowing the relative index.
TEST(MetadataPageTest, RejectsPageBelowTrackedRange) {
  MetadataPage page;
  page.setFirstTrackedPageId(10);

  EXPECT_THROW(page.setPageOccupied(9), std::out_of_range);
}

// Scenario 5: The invalid/sentinel PageId is rejected instead of indexing
// beyond the bitmap.
TEST(MetadataPageTest, RejectsInvalidSentinelPageId) {
  MetadataPage page;

  EXPECT_THROW(page.setPageOccupied(std::numeric_limits<uint32_t>::max()),
               std::out_of_range);
}

// Scenario 6: A PageId at the first position of a shifted tracked range is
// accepted.
TEST(MetadataPageTest, AcceptsFirstPageOfShiftedTrackedRange) {
  MetadataPage page;
  page.setFirstTrackedPageId(100);

  EXPECT_NO_THROW(page.setPageOccupied(100));
  EXPECT_EQ(page.findFirstFreePage(), 101);
}

// Scenario 7: Allocation returns distinct PageIds and persists each occupied
// bit through the metadata write port.
TEST(AllocatePageActionTest, AllocatesDistinctPagesAndPersistsMetadata) {
  MetadataMemoryStore store;
  store.pages.emplace(0, MetadataPage{});
  AllocatePageAction action(store, store);

  EXPECT_EQ(action.execute(), 1);
  EXPECT_EQ(action.execute(), 2);
  EXPECT_EQ(store.pages.at(0).findFirstFreePage(), 3);
  EXPECT_EQ(store.writeCount, 2);
}

// Scenario 8: A metadata write failure propagates and does not falsely report
// an allocation as successful.
TEST(AllocatePageActionTest,
     MetadataWriteFailureIsReportedAndRetryCanAllocateSamePage) {
  MetadataMemoryStore store;
  store.pages.emplace(0, MetadataPage{});
  AllocatePageAction action(store, store);
  store.failWrites = true;

  EXPECT_THROW(action.execute(), std::runtime_error);
  EXPECT_EQ(store.pages.at(0).findFirstFreePage(), 1);

  store.failWrites = false;
  EXPECT_EQ(action.execute(), 1);
  EXPECT_EQ(store.pages.at(0).findFirstFreePage(), 2);
}

// Scenario 9: When the root bitmap is full, allocation creates and links a
// new metadata page and reserves its own PageId.
TEST(AllocatePageActionTest, CreatesAndChainsMetadataPageWhenBitmapIsFull) {
  MetadataMemoryStore store;
  MetadataPage fullPage;
  for (uint32_t pageId = 1; pageId < MAX_TRACKED_PAGES; ++pageId) {
    fullPage.setPageOccupied(pageId);
  }
  store.pages.emplace(0, fullPage);
  AllocatePageAction action(store, store);

  const uint32_t newMetadataPageId = MAX_TRACKED_PAGES;
  const uint32_t allocatedPageId = newMetadataPageId + 1;
  EXPECT_EQ(action.execute(), allocatedPageId);
  EXPECT_EQ(store.pages.at(0).getNextMetadataPageId(), newMetadataPageId);
  EXPECT_EQ(store.pages.at(newMetadataPageId).getFirstTrackedPageId(),
            newMetadataPageId);
  EXPECT_EQ(store.pages.at(newMetadataPageId).findFirstFreePage(),
            allocatedPageId + 1);
}
