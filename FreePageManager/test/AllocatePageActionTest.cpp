#include "../actions/AllocatePageAction.h"
#include "../../core/Page.h"
#include "../domain/FreePageMetadata.h"

#include <gtest/gtest.h>
#include <map>
#include <stdexcept>

class FakeReadFreePageMetadataPort : public ReadFreePageMetadataPort {

public:
  std::map<PageId, Page> pages;

  void readPage(PageId pageId, Page &page) override { page = pages.at(pageId); }
};

class FakeWriteFreePageMetadataPort : public WriteFreePageMetadataPort {

public:
  std::map<PageId, Page> pages;

  void writePage(PageId pageId, const Page &page) override {

    pages[pageId] = page;
  }
};

TEST(AllocatePageActionTest, AllocatesFirstFreePage) {

  FakeReadFreePageMetadataPort readPort;
  FakeWriteFreePageMetadataPort writePort;

  FreePageMetadata metadata;

  Page metadataPage;
  metadata.writeToPage(metadataPage);

  readPort.pages[0] = metadataPage;

  AllocatePageAction action(readPort, writePort);

  PageId pageId = action.execute();

  EXPECT_EQ(pageId, 0);
}

TEST(AllocatePageActionTest, MarksAllocatedPageAsOccupied) {

  FakeReadFreePageMetadataPort readPort;
  FakeWriteFreePageMetadataPort writePort;

  FreePageMetadata metadata;

  Page metadataPage;
  metadata.writeToPage(metadataPage);

  readPort.pages[0] = metadataPage;

  AllocatePageAction action(readPort, writePort);

  PageId pageId = action.execute();

  EXPECT_EQ(pageId, 0);

  Page updatedMetadataPage = writePort.pages.at(0);

  FreePageMetadata updatedMetadata;

  updatedMetadata.readFromPage(updatedMetadataPage);

  EXPECT_TRUE(updatedMetadata.isPageOccupied(0));
}

TEST(AllocatePageActionTest, SkipsOccupiedPages) {

  FakeReadFreePageMetadataPort readPort;
  FakeWriteFreePageMetadataPort writePort;

  FreePageMetadata metadata;

  metadata.setPageOccupied(0, true);
  metadata.setPageOccupied(1, true);
  metadata.setPageOccupied(2, true);

  Page metadataPage;
  metadata.writeToPage(metadataPage);

  readPort.pages[0] = metadataPage;

  AllocatePageAction action(readPort, writePort);

  PageId pageId = action.execute();

  EXPECT_EQ(pageId, 3);
}

TEST(AllocatePageActionTest, WritesUpdatedMetadata) {

  FakeReadFreePageMetadataPort readPort;
  FakeWriteFreePageMetadataPort writePort;

  FreePageMetadata metadata;

  Page metadataPage;
  metadata.writeToPage(metadataPage);

  readPort.pages[0] = metadataPage;

  AllocatePageAction action(readPort, writePort);

  action.execute();

  EXPECT_TRUE(writePort.pages.find(0) != writePort.pages.end());
}

TEST(AllocatePageActionTest, AllocatesFromNextMetadataPage) {

  FakeReadFreePageMetadataPort readPort;
  FakeWriteFreePageMetadataPort writePort;

  FreePageMetadata firstMetadata;

  firstMetadata.setNextMetadataPageId(10);

  for (uint32_t offset = 0; offset < FreePageMetadata::MAX_TRACKED_PAGES;
       ++offset) {

    firstMetadata.setPageOccupied(offset, true);
  }

  Page firstMetadataPage;
  firstMetadata.writeToPage(firstMetadataPage);

  readPort.pages[0] = firstMetadataPage;

  FreePageMetadata secondMetadata;

  secondMetadata.setFirstTrackedPageId(FreePageMetadata::MAX_TRACKED_PAGES);

  Page secondMetadataPage;
  secondMetadata.writeToPage(secondMetadataPage);

  readPort.pages[10] = secondMetadataPage;

  AllocatePageAction action(readPort, writePort);

  PageId pageId = action.execute();

  EXPECT_EQ(pageId, FreePageMetadata::MAX_TRACKED_PAGES);
}

TEST(AllocatePageActionTest, WritesToCorrectMetadataPage) {

  FakeReadFreePageMetadataPort readPort;
  FakeWriteFreePageMetadataPort writePort;

  FreePageMetadata firstMetadata;

  firstMetadata.setNextMetadataPageId(10);

  for (uint32_t offset = 0; offset < FreePageMetadata::MAX_TRACKED_PAGES;
       ++offset) {

    firstMetadata.setPageOccupied(offset, true);
  }

  Page firstMetadataPage;
  firstMetadata.writeToPage(firstMetadataPage);

  readPort.pages[0] = firstMetadataPage;

  FreePageMetadata secondMetadata;

  secondMetadata.setFirstTrackedPageId(FreePageMetadata::MAX_TRACKED_PAGES);

  Page secondMetadataPage;
  secondMetadata.writeToPage(secondMetadataPage);

  readPort.pages[10] = secondMetadataPage;

  AllocatePageAction action(readPort, writePort);

  PageId pageId = action.execute();

  EXPECT_EQ(pageId, FreePageMetadata::MAX_TRACKED_PAGES);

  EXPECT_TRUE(writePort.pages.find(10) != writePort.pages.end());

  EXPECT_TRUE(writePort.pages.find(0) == writePort.pages.end());
}

TEST(AllocatePageActionTest, AllocatedPageIsOccupiedAfterWriting) {

  FakeReadFreePageMetadataPort readPort;
  FakeWriteFreePageMetadataPort writePort;

  FreePageMetadata metadata;

  metadata.setPageOccupied(0, true);
  metadata.setPageOccupied(1, true);

  Page metadataPage;
  metadata.writeToPage(metadataPage);

  readPort.pages[0] = metadataPage;

  AllocatePageAction action(readPort, writePort);

  PageId pageId = action.execute();

  EXPECT_EQ(pageId, 2);

  Page updatedPage = writePort.pages.at(0);

  FreePageMetadata updatedMetadata;

  updatedMetadata.readFromPage(updatedPage);

  EXPECT_TRUE(updatedMetadata.isPageOccupied(2));

  EXPECT_TRUE(updatedMetadata.isPageOccupied(0));

  EXPECT_TRUE(updatedMetadata.isPageOccupied(1));

  EXPECT_FALSE(updatedMetadata.isPageOccupied(3));
}

TEST(AllocatePageActionTest, ThrowsWhenAllMetadataPagesAreFull) {

  FakeReadFreePageMetadataPort readPort;
  FakeWriteFreePageMetadataPort writePort;

  FreePageMetadata metadata;

  for (uint32_t offset = 0; offset < FreePageMetadata::MAX_TRACKED_PAGES;
       ++offset) {

    metadata.setPageOccupied(offset, true);
  }

  Page metadataPage;
  metadata.writeToPage(metadataPage);

  readPort.pages[0] = metadataPage;

  AllocatePageAction action(readPort, writePort);

  EXPECT_THROW(action.execute(), std::runtime_error);
}

TEST(AllocatePageActionTest, ThrowsWhenMetadataChainEnds) {

  FakeReadFreePageMetadataPort readPort;
  FakeWriteFreePageMetadataPort writePort;

  FreePageMetadata metadata;

  for (uint32_t offset = 0; offset < FreePageMetadata::MAX_TRACKED_PAGES;
       ++offset) {

    metadata.setPageOccupied(offset, true);
  }

  metadata.setNextMetadataPageId(FreePageMetadata::INVALID_PAGE_ID);

  Page metadataPage;
  metadata.writeToPage(metadataPage);

  readPort.pages[0] = metadataPage;

  AllocatePageAction action(readPort, writePort);

  EXPECT_THROW(action.execute(), std::runtime_error);
}
