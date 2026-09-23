#pragma once

#include "../../core/Page.h"
#include "../../core/PageId.h"
#include "../../core/TableId.h"
#include "Table.h"

#include <cstdint>
#include <vector>

class CatalogPage {
public:
  static constexpr PageId INVALID_PAGE_ID = UINT32_MAX;

  static constexpr uint16_t MAX_TABLES_PER_PAGE = 8;
  static constexpr uint16_t MAX_COLUMNS_PER_TABLE = 16;

  static constexpr uint16_t MAX_TABLE_NAME_LENGTH = 64;
  static constexpr uint16_t MAX_COLUMN_NAME_LENGTH = 32;

  CatalogPage();

  PageId getNextPageId() const;
  void setNextPageId(PageId pageId);

  uint16_t getTableCount() const;

  const std::vector<Table> &getTables() const;

  void addTable(const Table &table);
  void removeTable(TableId tableId);

  Table *findTable(TableId tableId);
  const Table *findTable(TableId tableId) const;

  void readFromPage(const Page &page);
  void writeToPage(Page &page) const;

private:
  PageId nextPageId;
  std::vector<Table> tables;
};