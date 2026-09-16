#include "../domain/FreePageMetadata.h"

#include "../../core/Page.h"

#include <gtest/gtest.h>

TEST(FreePageMetadataTest, InitializesWithInvalidNextMetadataPage) {
  FreePageMetadata metadata;

  EXPECT_EQ(metadata.getNextMetadataPageId(),
            FreePageMetadata::INVALID_PAGE_ID);
}

TEST(FreePageMetadataTest, InitializesWithFirstTrackedPageIdZero) {
  FreePageMetadata metadata;

  EXPECT_EQ(metadata.getFirstTrackedPageId(), 0);
}

TEST(FreePageMetadataTest, SetAndGetNextMetadataPageId) {
  FreePageMetadata metadata;

  metadata.setNextMetadataPageId(10);

  EXPECT_EQ(metadata.getNextMetadataPageId(), 10);
}

TEST(FreePageMetadataTest, SetAndGetFirstTrackedPageId) {
  FreePageMetadata metadata;

  metadata.setFirstTrackedPageId(65504);

  EXPECT_EQ(metadata.getFirstTrackedPageId(), 65504);
}

TEST(FreePageMetadataTest, PagesAreInitiallyFree) {
  FreePageMetadata metadata;

  EXPECT_FALSE(metadata.isPageOccupied(0));
  EXPECT_FALSE(metadata.isPageOccupied(1));
  EXPECT_FALSE(metadata.isPageOccupied(100));
}

TEST(FreePageMetadataTest, CanMarkPageOccupied) {
  FreePageMetadata metadata;

  metadata.setPageOccupied(5, true);

  EXPECT_TRUE(metadata.isPageOccupied(5));
}

TEST(FreePageMetadataTest, CanMarkPageFree) {
  FreePageMetadata metadata;

  metadata.setPageOccupied(5, true);
  metadata.setPageOccupied(5, false);

  EXPECT_FALSE(metadata.isPageOccupied(5));
}

TEST(FreePageMetadataTest, OccupyingOnePageDoesNotAffectOtherPages) {
  FreePageMetadata metadata;

  metadata.setPageOccupied(5, true);

  EXPECT_TRUE(metadata.isPageOccupied(5));
  EXPECT_FALSE(metadata.isPageOccupied(4));
  EXPECT_FALSE(metadata.isPageOccupied(6));
}

TEST(FreePageMetadataTest, CanTrackMultiplePagesInSameByte) {
  FreePageMetadata metadata;

  metadata.setPageOccupied(0, true);
  metadata.setPageOccupied(1, true);
  metadata.setPageOccupied(7, true);

  EXPECT_TRUE(metadata.isPageOccupied(0));
  EXPECT_TRUE(metadata.isPageOccupied(1));
  EXPECT_TRUE(metadata.isPageOccupied(7));

  EXPECT_FALSE(metadata.isPageOccupied(2));
  EXPECT_FALSE(metadata.isPageOccupied(6));
}

TEST(FreePageMetadataTest, CanTrackPagesAcrossDifferentBytes) {
  FreePageMetadata metadata;

  metadata.setPageOccupied(0, true);
  metadata.setPageOccupied(8, true);
  metadata.setPageOccupied(16, true);

  EXPECT_TRUE(metadata.isPageOccupied(0));
  EXPECT_TRUE(metadata.isPageOccupied(8));
  EXPECT_TRUE(metadata.isPageOccupied(16));

  EXPECT_FALSE(metadata.isPageOccupied(1));
  EXPECT_FALSE(metadata.isPageOccupied(9));
  EXPECT_FALSE(metadata.isPageOccupied(17));
}

TEST(FreePageMetadataTest, FindFirstFreePage) {
  FreePageMetadata metadata;

  metadata.setPageOccupied(0, true);
  metadata.setPageOccupied(1, true);
  metadata.setPageOccupied(2, true);

  EXPECT_EQ(metadata.findFirstFreePage(), 3);
}

TEST(FreePageMetadataTest, FindFirstFreePageWithFirstTrackedPageId) {
  FreePageMetadata metadata;

  metadata.setFirstTrackedPageId(100);

  metadata.setPageOccupied(100, true);
  metadata.setPageOccupied(101, true);

  EXPECT_EQ(metadata.findFirstFreePage(), 102);
}

TEST(FreePageMetadataTest,
     FindFirstFreePageReturnsInvalidWhenAllPagesOccupied) {
  FreePageMetadata metadata;

  for (uint32_t offset = 0; offset < FreePageMetadata::MAX_TRACKED_PAGES;
       ++offset) {

    metadata.setPageOccupied(offset, true);
  }

  EXPECT_EQ(metadata.findFirstFreePage(), FreePageMetadata::INVALID_PAGE_ID);
}

TEST(FreePageMetadataTest, WriteToPageAndReadFromPage) {
  FreePageMetadata original;

  original.setNextMetadataPageId(50);
  original.setFirstTrackedPageId(100);

  original.setPageOccupied(100, true);
  original.setPageOccupied(105, true);
  original.setPageOccupied(200, true);

  Page page;

  original.writeToPage(page);

  FreePageMetadata restored;

  restored.readFromPage(page);

  EXPECT_EQ(restored.getNextMetadataPageId(), 50);

  EXPECT_EQ(restored.getFirstTrackedPageId(), 100);

  EXPECT_TRUE(restored.isPageOccupied(100));
  EXPECT_TRUE(restored.isPageOccupied(105));
  EXPECT_TRUE(restored.isPageOccupied(200));

  EXPECT_FALSE(restored.isPageOccupied(101));
  EXPECT_FALSE(restored.isPageOccupied(106));
}

TEST(FreePageMetadataTest, SerializationPreservesFirstFreePage) {
  FreePageMetadata original;

  original.setFirstTrackedPageId(100);

  original.setPageOccupied(100, true);
  original.setPageOccupied(101, true);
  original.setPageOccupied(102, true);

  Page page;

  original.writeToPage(page);

  FreePageMetadata restored;

  restored.readFromPage(page);

  EXPECT_EQ(restored.findFirstFreePage(), 103);
}