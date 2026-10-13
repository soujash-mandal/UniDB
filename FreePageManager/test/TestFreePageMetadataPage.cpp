#include "../domain/MetadataPage.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <stdexcept>

// Scenario 1: A newly initialized metadata page reserves page 0 and returns page 1 as the first free page.
TEST(MetadataPageTest, ReservesMetadataPageAndFindsFirstFreePage) {
  MetadataPage page;

  EXPECT_EQ(page.getFirstTrackedPageId(), 0);
  EXPECT_EQ(page.findFirstFreePage(), 1);
}

// Scenario 2: Marking the current first free page occupied advances allocation to the next free PageId.
TEST(MetadataPageTest, MarkingPageOccupiedAdvancesFirstFreePage) {
  MetadataPage page;

  page.setPageOccupied(1);

  EXPECT_EQ(page.findFirstFreePage(), 2);
}

// Scenario 3: Bitmap tracking supports the last valid bit and reports exhaustion after every tracked bit is occupied.
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

// Scenario 4: A PageId below firstTrackedPageId is rejected instead of underflowing the relative index.
TEST(MetadataPageTest, RejectsPageBelowTrackedRange) {
  MetadataPage page;
  page.setFirstTrackedPageId(10);

  EXPECT_THROW(page.setPageOccupied(9), std::out_of_range);
}

// Scenario 5: The invalid/sentinel PageId is rejected instead of indexing beyond the bitmap.
TEST(MetadataPageTest, RejectsInvalidSentinelPageId) {
  MetadataPage page;

  EXPECT_THROW(page.setPageOccupied(std::numeric_limits<uint32_t>::max()),
               std::out_of_range);
}

// Scenario 6: A PageId at the first position of a shifted tracking range is accepted and tracked.
TEST(MetadataPageTest, AcceptsFirstPageOfShiftedTrackedRange) {
  MetadataPage page;
  page.setFirstTrackedPageId(100);

  EXPECT_NO_THROW(page.setPageOccupied(100));
  EXPECT_EQ(page.findFirstFreePage(), 101);
}
