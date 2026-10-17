#include "TablePageDirectoryPage.h"

#include <cstring>
#include <stdexcept>

namespace {
struct Header {
  TablePageDirectoryPageId nextPageId;
  uint32_t entryCount;
};

struct Entry {
  TuplePageId pageId;
  uint32_t freeSpace;
};

constexpr uint32_t HEADER_SIZE = sizeof(Header);
constexpr uint32_t ENTRY_SIZE = sizeof(Entry);
constexpr uint32_t MAX_ENTRIES =
    (TablePageDirectoryPage::PAGE_SIZE - HEADER_SIZE) / ENTRY_SIZE;

Header readHeader(const char *bytes) {
  Header header{};
  std::memcpy(&header, bytes, sizeof(header));
  if (header.entryCount > MAX_ENTRIES) {
    throw std::runtime_error("Invalid TablePageDirectoryPage entry count");
  }
  return header;
}

Entry readEntry(const char *bytes, uint32_t index) {
  Entry entry{};
  std::memcpy(&entry, bytes + HEADER_SIZE + index * ENTRY_SIZE,
              sizeof(entry));
  return entry;
}
} // namespace

TablePageDirectoryPage::TablePageDirectoryPage() {
  std::memset(bytes, 0, PAGE_SIZE);
  Header header{TABLE_PAGE_DIRECTORY_INVALID_PAGE_ID, 0};
  std::memcpy(bytes, &header, sizeof(header));
}

TablePageDirectoryPageId TablePageDirectoryPage::getNextPageId() const {
  return readHeader(bytes).nextPageId;
}

void TablePageDirectoryPage::setNextPageId(
    TablePageDirectoryPageId pageId) {
  Header header = readHeader(bytes);
  header.nextPageId = pageId;
  std::memcpy(bytes, &header, sizeof(header));
}

bool TablePageDirectoryPage::isFull() const {
  return readHeader(bytes).entryCount >= MAX_ENTRIES;
}

uint32_t TablePageDirectoryPage::size() const {
  return readHeader(bytes).entryCount;
}

bool TablePageDirectoryPage::contains(TuplePageId pageId) const {
  const Header header = readHeader(bytes);
  for (uint32_t i = 0; i < header.entryCount; ++i) {
    if (readEntry(bytes, i).pageId == pageId) {
      return true;
    }
  }
  return false;
}

uint32_t
TablePageDirectoryPage::getFreeSpace(TuplePageId pageId) const {
  const Header header = readHeader(bytes);
  for (uint32_t i = 0; i < header.entryCount; ++i) {
    const Entry entry = readEntry(bytes, i);
    if (entry.pageId == pageId) {
      return entry.freeSpace;
    }
  }
  throw std::runtime_error("Tuple page not found in TablePageDirectoryPage");
}

TuplePageId
TablePageDirectoryPage::findPageWithSpace(uint32_t requiredSpace) const {
  const Header header = readHeader(bytes);
  for (uint32_t i = 0; i < header.entryCount; ++i) {
    const Entry entry = readEntry(bytes, i);
    if (entry.freeSpace >= requiredSpace) {
      return entry.pageId;
    }
  }
  return TABLE_PAGE_DIRECTORY_INVALID_PAGE_ID;
}

void TablePageDirectoryPage::insert(TuplePageId pageId, uint32_t freeSpace) {
  if (pageId == TABLE_PAGE_DIRECTORY_INVALID_PAGE_ID) {
    throw std::runtime_error("Cannot register an invalid tuple page id");
  }
  if (freeSpace > PAGE_SIZE) {
    throw std::runtime_error("Tuple page free space exceeds page size");
  }

  Header header = readHeader(bytes);
  for (uint32_t i = 0; i < header.entryCount; ++i) {
    if (readEntry(bytes, i).pageId == pageId) {
      throw std::runtime_error("Tuple page already registered");
    }
  }
  if (header.entryCount >= MAX_ENTRIES) {
    throw std::runtime_error("TablePageDirectoryPage is full");
  }

  const Entry entry{pageId, freeSpace};
  std::memcpy(bytes + HEADER_SIZE + header.entryCount * ENTRY_SIZE, &entry,
              sizeof(entry));
  ++header.entryCount;
  std::memcpy(bytes, &header, sizeof(header));
}

void TablePageDirectoryPage::update(TuplePageId pageId, uint32_t freeSpace) {
  if (freeSpace > PAGE_SIZE) {
    throw std::runtime_error("Tuple page free space exceeds page size");
  }

  const Header header = readHeader(bytes);
  for (uint32_t i = 0; i < header.entryCount; ++i) {
    Entry entry = readEntry(bytes, i);
    if (entry.pageId == pageId) {
      entry.freeSpace = freeSpace;
      std::memcpy(bytes + HEADER_SIZE + i * ENTRY_SIZE, &entry,
                  sizeof(entry));
      return;
    }
  }
  throw std::runtime_error("Tuple page not found in TablePageDirectoryPage");
}
