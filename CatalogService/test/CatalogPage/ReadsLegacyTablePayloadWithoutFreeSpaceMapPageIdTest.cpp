#include "CatalogPageTestSupport.h"

#include <gtest/gtest.h>

#include <cstring>

TEST(CatalogPageTest, ReadsLegacyTablePayloadWithoutFreeSpaceMapPageId) {
  CatalogPage page;
  Table table = makeCatalogPageTestTable(31, "legacy");
  table.firstFreeSpaceMapPageId = 77;
  page.insert(table);
  ASSERT_EQ(page.get(31).firstFreeSpaceMapPageId, 77U);

  // Scenario 1: Simulate the old payload format by excluding its final field.
  uint32_t payloadSize = 0;
  std::memcpy(&payloadSize, page.data() + sizeof(uint32_t) * 5,
              sizeof(payloadSize));
  payloadSize -= sizeof(FSMPageId);
  std::memcpy(page.data() + sizeof(uint32_t) * 5, &payloadSize,
              sizeof(payloadSize));

  const Table loaded = page.get(31);
  EXPECT_EQ(loaded.firstFreeSpaceMapPageId, CATALOG_INVALID_PAGE_ID);
}
