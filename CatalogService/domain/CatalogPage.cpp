#include "CatalogPage.h"

#include <cstring>
#include <stdexcept>

namespace {

#pragma pack(push, 1)

struct SerializedColumn {
  char name[CatalogPage::MAX_COLUMN_NAME_LENGTH];
  uint8_t type;
  uint16_t size;
  uint8_t nullable;
};

struct SerializedTable {
  TableId tableId;
  char name[CatalogPage::MAX_TABLE_NAME_LENGTH];
  uint16_t columnCount;
  PageId firstFreeSpaceMapPageId;
  SerializedColumn columns[CatalogPage::MAX_COLUMNS_PER_TABLE];
};

struct SerializedCatalogPageHeader {
  PageId nextPageId;
  uint16_t tableCount;
};

#pragma pack(pop)

constexpr std::size_t SERIALIZED_SIZE =
    sizeof(SerializedCatalogPageHeader) +
    sizeof(SerializedTable) * CatalogPage::MAX_TABLES_PER_PAGE;

static_assert(SERIALIZED_SIZE <= Page::PAGE_SIZE,
              "CatalogPage serialized format does not fit in a page");

void copyStringToBuffer(char *destination, std::size_t destinationSize,
                        const std::string &source) {
  if (source.size() >= destinationSize) {
    throw std::invalid_argument("String is too long for catalog record");
  }

  std::memset(destination, 0, destinationSize);
  std::memcpy(destination, source.data(), source.size());
}

std::string copyBufferToString(const char *source, std::size_t size) {
  std::size_t length = 0;

  while (length < size && source[length] != '\0') {
    ++length;
  }

  return std::string(source, length);
}

} // namespace

CatalogPage::CatalogPage() : nextPageId(INVALID_PAGE_ID), tables() {}

PageId CatalogPage::getNextPageId() const { return nextPageId; }

void CatalogPage::setNextPageId(PageId pageId) { nextPageId = pageId; }

uint16_t CatalogPage::getTableCount() const {
  return static_cast<uint16_t>(tables.size());
}

const std::vector<Table> &CatalogPage::getTables() const { return tables; }

void CatalogPage::addTable(const Table &table) {
  if (tables.size() >= MAX_TABLES_PER_PAGE) {
    throw std::runtime_error("Catalog page is full");
  }

  if (table.name.size() >= MAX_TABLE_NAME_LENGTH) {
    throw std::invalid_argument("Table name is too long");
  }

  if (table.columns.size() > MAX_COLUMNS_PER_TABLE) {
    throw std::invalid_argument("Too many columns in table");
  }

  for (const Column &column : table.columns) {
    if (column.name.size() >= MAX_COLUMN_NAME_LENGTH) {
      throw std::invalid_argument("Column name is too long");
    }
  }

  if (findTable(table.tableId) != nullptr) {
    throw std::invalid_argument("Table already exists in catalog page");
  }

  tables.push_back(table);
}

void CatalogPage::removeTable(TableId tableId) {
  for (auto it = tables.begin(); it != tables.end(); ++it) {
    if (it->tableId == tableId) {
      tables.erase(it);
      return;
    }
  }

  throw std::invalid_argument("Table not found");
}

Table *CatalogPage::findTable(TableId tableId) {
  for (Table &table : tables) {
    if (table.tableId == tableId) {
      return &table;
    }
  }

  return nullptr;
}

const Table *CatalogPage::findTable(TableId tableId) const {
  for (const Table &table : tables) {
    if (table.tableId == tableId) {
      return &table;
    }
  }

  return nullptr;
}

void CatalogPage::readFromPage(const Page &page) {
  SerializedCatalogPageHeader header;

  std::memcpy(&header, page.data(), sizeof(header));

  if (header.tableCount > MAX_TABLES_PER_PAGE) {
    throw std::runtime_error("Invalid catalog page: table count is too large");
  }

  nextPageId = header.nextPageId;
  tables.clear();

  const char *pageData = page.data() + sizeof(header);

  for (uint16_t i = 0; i < header.tableCount; ++i) {
    SerializedTable serializedTable;

    std::memcpy(&serializedTable, pageData + i * sizeof(SerializedTable),
                sizeof(SerializedTable));

    Table table;

    table.tableId = serializedTable.tableId;

    table.name =
        copyBufferToString(serializedTable.name, MAX_TABLE_NAME_LENGTH);

    if (serializedTable.columnCount > MAX_COLUMNS_PER_TABLE) {
      throw std::runtime_error(
          "Invalid catalog page: column count is too large");
    }

    table.firstFreeSpaceMapPageId = serializedTable.firstFreeSpaceMapPageId;

    for (uint16_t j = 0; j < serializedTable.columnCount; ++j) {
      const SerializedColumn &serializedColumn = serializedTable.columns[j];

      Column column;

      column.name =
          copyBufferToString(serializedColumn.name, MAX_COLUMN_NAME_LENGTH);

      column.type = static_cast<DataType>(serializedColumn.type);

      column.size = serializedColumn.size;
      column.nullable = serializedColumn.nullable != 0;

      table.columns.push_back(column);
    }

    tables.push_back(table);
  }
}

void CatalogPage::writeToPage(Page &page) const {
  if (tables.size() > MAX_TABLES_PER_PAGE) {
    throw std::runtime_error("Too many tables for catalog page");
  }

  std::memset(page.data(), 0, Page::PAGE_SIZE);

  SerializedCatalogPageHeader header;

  header.nextPageId = nextPageId;
  header.tableCount = static_cast<uint16_t>(tables.size());

  std::memcpy(page.data(), &header, sizeof(header));

  char *pageData = page.data() + sizeof(header);

  for (std::size_t i = 0; i < tables.size(); ++i) {
    const Table &table = tables[i];

    if (table.name.size() >= MAX_TABLE_NAME_LENGTH) {
      throw std::invalid_argument("Table name is too long");
    }

    if (table.columns.size() > MAX_COLUMNS_PER_TABLE) {
      throw std::invalid_argument("Too many columns in table");
    }

    SerializedTable serializedTable{};

    serializedTable.tableId = table.tableId;

    copyStringToBuffer(serializedTable.name, MAX_TABLE_NAME_LENGTH, table.name);

    serializedTable.columnCount = static_cast<uint16_t>(table.columns.size());

    serializedTable.firstFreeSpaceMapPageId = table.firstFreeSpaceMapPageId;

    for (std::size_t j = 0; j < table.columns.size(); ++j) {
      const Column &column = table.columns[j];

      if (column.name.size() >= MAX_COLUMN_NAME_LENGTH) {
        throw std::invalid_argument("Column name is too long");
      }

      SerializedColumn &serializedColumn = serializedTable.columns[j];

      copyStringToBuffer(serializedColumn.name, MAX_COLUMN_NAME_LENGTH,
                         column.name);

      serializedColumn.type = static_cast<uint8_t>(column.type);

      serializedColumn.size = column.size;
      serializedColumn.nullable = column.nullable ? 1 : 0;
    }

    std::memcpy(pageData + i * sizeof(SerializedTable), &serializedTable,
                sizeof(SerializedTable));
  }
}
