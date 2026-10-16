#include "CatalogPage.h"

#include <cstring>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {
std::vector<char> serializeTable(Table table) {
  std::vector<char> data;
  auto append = [&data](const void *value, std::size_t size) {
    const char *bytes = static_cast<const char *>(value);
    data.insert(data.end(), bytes, bytes + size);
  };
  uint32_t nameSize = static_cast<uint32_t>(table.name.size());
  append(&table.tableId, sizeof(table.tableId));
  append(&nameSize, sizeof(nameSize));
  append(table.name.data(), nameSize);
  uint32_t columnCount = static_cast<uint32_t>(table.columns.size());
  append(&columnCount, sizeof(columnCount));
  for (const Column &column : table.columns) {
    uint32_t columnNameSize = static_cast<uint32_t>(column.name.size());
    append(&columnNameSize, sizeof(columnNameSize));
    append(column.name.data(), columnNameSize);
    uint8_t type = static_cast<uint8_t>(column.type);
    append(&type, sizeof(type));
    append(&column.size, sizeof(column.size));
    uint8_t nullable = column.nullable ? 1 : 0;
    append(&nullable, sizeof(nullable));
  }
  append(&table.firstFreeSpaceMapPageId, sizeof(table.firstFreeSpaceMapPageId));
  append(&table.firstTuplePageId, sizeof(table.firstTuplePageId));
  return data;
}

Table deserializeTable(std::vector<char> data) {
  Table table;
  std::size_t offset = 0;
  auto read = [&data, &offset](void *destination, std::size_t size) {
    if (offset + size > data.size()) {
      throw std::runtime_error("Invalid catalog table data");
    }
    std::memcpy(destination, data.data() + offset, size);
    offset += size;
  };
  uint32_t nameSize;
  uint32_t columnCount;
  read(&table.tableId, sizeof(table.tableId));
  read(&nameSize, sizeof(nameSize));
  if (offset + nameSize > data.size()) {
    throw std::runtime_error("Invalid catalog table name");
  }
  table.name.assign(data.data() + offset, nameSize);
  offset += nameSize;
  read(&columnCount, sizeof(columnCount));
  table.columns.reserve(columnCount);
  for (uint32_t i = 0; i < columnCount; ++i) {
    Column column;
    uint32_t columnNameSize;
    uint8_t type;
    uint8_t nullable;
    read(&columnNameSize, sizeof(columnNameSize));
    if (offset + columnNameSize > data.size()) {
      throw std::runtime_error("Invalid catalog column name");
    }
    column.name.assign(data.data() + offset, columnNameSize);
    offset += columnNameSize;
    read(&type, sizeof(type));
    column.type = static_cast<DataType>(type);
    read(&column.size, sizeof(column.size));
    read(&nullable, sizeof(nullable));
    column.nullable = nullable != 0;
    table.columns.push_back(column);
  }
  read(&table.firstFreeSpaceMapPageId, sizeof(table.firstFreeSpaceMapPageId));
  if (offset < data.size()) {
    read(&table.firstTuplePageId, sizeof(table.firstTuplePageId));
  } else {
    table.firstTuplePageId = CATALOG_INVALID_PAGE_ID;
  }
  return table;
}
} // namespace

CatalogPage::CatalogPage() {
  std::memset(bytes, 0, PAGE_SIZE);
  Header header;
  header.nextPageId = CATALOG_INVALID_PAGE_ID;
  header.tableCount = 0;
  header.freeSpaceOffset = PAGE_SIZE;
  std::memcpy(bytes, &header, sizeof(Header));
}

CatalogPageId CatalogPage::getNextPageId() {
  Header header;
  std::memcpy(&header, bytes, sizeof(Header));
  return header.nextPageId;
}

void CatalogPage::setNextPageId(CatalogPageId pageId) {
  Header header;
  std::memcpy(&header, bytes, sizeof(Header));
  header.nextPageId = pageId;
  std::memcpy(bytes, &header, sizeof(Header));
}

void CatalogPage::insert(Table table) {
  std::vector<char> tableData = serializeTable(table);
  Header header;
  std::memcpy(&header, bytes, sizeof(Header));
  for (uint32_t i = 0; i < header.tableCount; ++i) {
    Entry entry;
    std::memcpy(&entry, bytes + sizeof(Header) + i * sizeof(Entry),
                sizeof(Entry));
    if (entry.tableId == table.tableId) {
      throw std::runtime_error("Table already exists");
    }
  }
  if (!hasSpace(table)) {
    throw std::runtime_error("Not enough space in CatalogPage");
  }
  uint32_t newOffset =
      header.freeSpaceOffset - static_cast<uint32_t>(tableData.size());
  uint32_t entryOffset = sizeof(Header) + header.tableCount * sizeof(Entry);
  Entry entry;
  entry.tableId = table.tableId;
  entry.offset = newOffset;
  entry.size = static_cast<uint32_t>(tableData.size());
  std::memcpy(bytes + newOffset, tableData.data(), tableData.size());
  std::memcpy(bytes + entryOffset, &entry, sizeof(Entry));
  ++header.tableCount;
  header.freeSpaceOffset = newOffset;
  std::memcpy(bytes, &header, sizeof(Header));
}

Table CatalogPage::get(CatalogTableId tableId) {
  Header header;
  std::memcpy(&header, bytes, sizeof(Header));
  for (uint32_t i = 0; i < header.tableCount; ++i) {
    Entry entry;
    std::memcpy(&entry, bytes + sizeof(Header) + i * sizeof(Entry),
                sizeof(Entry));
    if (entry.tableId == tableId) {
      std::vector<char> tableData(bytes + entry.offset,
                                  bytes + entry.offset + entry.size);
      return deserializeTable(tableData);
    }
  }
  throw std::runtime_error("Table not found");
}

Table CatalogPage::getByName(const std::string &name) {
  Header header;
  std::memcpy(&header, bytes, sizeof(Header));
  for (uint32_t i = 0; i < header.tableCount; ++i) {
    Entry entry;
    std::memcpy(&entry, bytes + sizeof(Header) + i * sizeof(Entry),
                sizeof(Entry));
    std::vector<char> tableData(bytes + entry.offset,
                                bytes + entry.offset + entry.size);
    Table table = deserializeTable(tableData);
    if (table.name == name) {
      return table;
    }
  }
  throw std::runtime_error("Table not found");
}

void CatalogPage::update(Table table) {
  std::vector<char> replacement = serializeTable(table);
  Header header;
  std::memcpy(&header, bytes, sizeof(Header));

  std::vector<std::pair<Entry, std::vector<char>>> tables;
  tables.reserve(header.tableCount);
  bool found = false;
  std::size_t payloadBytes = 0;

  for (uint32_t i = 0; i < header.tableCount; ++i) {
    Entry entry;
    std::memcpy(&entry, bytes + sizeof(Header) + i * sizeof(Entry),
                sizeof(Entry));
    std::vector<char> payload;
    if (entry.tableId == table.tableId) {
      if (found) {
        throw std::runtime_error("Duplicate catalog table id");
      }
      found = true;
      payload = replacement;
      entry.size = static_cast<uint32_t>(payload.size());
    } else {
      payload.assign(bytes + entry.offset, bytes + entry.offset + entry.size);
    }
    payloadBytes += payload.size();
    tables.emplace_back(entry, std::move(payload));
  }

  if (!found) {
    throw std::runtime_error("Table not found");
  }
  const std::size_t entryEnd =
      sizeof(Header) + tables.size() * sizeof(Entry);
  if (entryEnd + payloadBytes > PAGE_SIZE) {
    throw std::runtime_error("Not enough space to update catalog table");
  }

  // Repack live payloads so repeated updates reclaim old payload space.
  std::memset(bytes, 0, PAGE_SIZE);
  uint32_t freeSpaceOffset = PAGE_SIZE;
  for (std::size_t i = 0; i < tables.size(); ++i) {
    auto &entry = tables[i].first;
    const auto &payload = tables[i].second;
    freeSpaceOffset -= static_cast<uint32_t>(payload.size());
    std::memcpy(bytes + freeSpaceOffset, payload.data(), payload.size());
    entry.offset = freeSpaceOffset;
    entry.size = static_cast<uint32_t>(payload.size());
    std::memcpy(bytes + sizeof(Header) + i * sizeof(Entry), &entry,
                sizeof(Entry));
  }
  header.freeSpaceOffset = freeSpaceOffset;
  std::memcpy(bytes, &header, sizeof(Header));
}

bool CatalogPage::containsTableName(std::string name) {
  Header header;
  std::memcpy(&header, bytes, sizeof(Header));
  for (uint32_t i = 0; i < header.tableCount; ++i) {
    Entry entry;
    std::memcpy(&entry, bytes + sizeof(Header) + i * sizeof(Entry),
                sizeof(Entry));
    std::vector<char> tableData(bytes + entry.offset,
                                bytes + entry.offset + entry.size);
    Table table = deserializeTable(tableData);
    if (table.name == name) {
      return true;
    }
  }
  return false;
}

bool CatalogPage::containsTableId(CatalogTableId tableId) {
  Header header;
  std::memcpy(&header, bytes, sizeof(Header));
  for (uint32_t i = 0; i < header.tableCount; ++i) {
    Entry entry;
    std::memcpy(&entry, bytes + sizeof(Header) + i * sizeof(Entry),
                sizeof(Entry));
    if (entry.tableId == tableId) {
      return true;
    }
  }
  return false;
}

void CatalogPage::remove(CatalogTableId tableId) {
  // Step 1: Read the header so we know how many entries to inspect.
  Header header;
  std::memcpy(&header, bytes, sizeof(Header));

  // Step 2: Find the entry for the table we want to delete.
  // Using tableCount as the "not found" marker keeps this step simple.
  uint32_t targetIndex = header.tableCount;

  for (uint32_t i = 0; i < header.tableCount; ++i) {
    Entry entry;
    std::memcpy(&entry, bytes + sizeof(Header) + i * sizeof(Entry),
                sizeof(Entry));

    if (entry.tableId == tableId) {
      targetIndex = i;
      break;
    }
  }

  // If we checked every entry and found no match, report the error.
  if (targetIndex == header.tableCount) {
    throw std::runtime_error("Table not found");
  }

  // Step 3: Copy every table except the one being deleted.
  // We take these copies before changing the page, so compaction cannot
  // overwrite payload bytes that we still need to keep.
  std::vector<std::pair<Entry, std::vector<char>>> remainingTables;

  for (uint32_t i = 0; i < header.tableCount; ++i) {
    if (i == targetIndex) {
      continue; // Skip the table requested for deletion.
    }

    Entry entry;
    std::memcpy(&entry, bytes + sizeof(Header) + i * sizeof(Entry),
                sizeof(Entry));

    std::vector<char> payload(bytes + entry.offset,
                              bytes + entry.offset + entry.size);

    remainingTables.emplace_back(entry, std::move(payload));
  }

  // Step 4: Write the remaining payloads back, starting at the page's end.
  // Each payload gets a new offset because the deleted table left a gap.
  uint32_t newFreeSpaceOffset = PAGE_SIZE;
  uint32_t newTableCount = 0;

  for (auto &table : remainingTables) {
    Entry entry = table.first;
    const std::vector<char> &payload = table.second;

    newFreeSpaceOffset -= entry.size;
    std::memcpy(bytes + newFreeSpaceOffset, payload.data(), payload.size());

    // Store the payload's new location in its entry.
    entry.offset = newFreeSpaceOffset;

    // Keep the entry array compact, with no gap for the deleted table.
    std::memcpy(bytes + sizeof(Header) + newTableCount * sizeof(Entry), &entry,
                sizeof(Entry));
    ++newTableCount;
  }

  // Step 5: Clear entry slots left over at the end of the old entry array.
  const uint32_t removedEntryCount = header.tableCount - newTableCount;
  std::memset(bytes + sizeof(Header) + newTableCount * sizeof(Entry), 0,
              removedEntryCount * sizeof(Entry));

  // Step 6: Save the updated table count and free-space boundary.
  header.tableCount = newTableCount;
  header.freeSpaceOffset = newFreeSpaceOffset;
  std::memcpy(bytes, &header, sizeof(Header));
}

bool CatalogPage::hasSpace(Table table) {
  std::vector<char> tableData = serializeTable(table);
  Header header;
  std::memcpy(&header, bytes, sizeof(Header));
  uint32_t entryEnd = sizeof(Header) + (header.tableCount + 1) * sizeof(Entry);
  if (header.freeSpaceOffset < tableData.size()) {
    return false;
  }
  uint32_t newDataStart =
      header.freeSpaceOffset - static_cast<uint32_t>(tableData.size());
  return newDataStart >= entryEnd;
}
