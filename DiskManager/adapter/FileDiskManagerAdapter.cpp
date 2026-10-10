#include "FileDiskManagerAdapter.h"

#include <fstream>
#include <stdexcept>

DiskPage FileDiskManagerAdapter::readDiskPage(DiskPageId pageId) {
  std::fstream file(fileName, std::ios::in | std::ios::out | std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("Could not open existing database file");
  }

  DiskPage page;
  const std::streamoff offset =
      static_cast<std::streamoff>(pageId) * DiskPage::PAGE_SIZE;
  file.seekg(offset);
  if (!file) {
    throw std::runtime_error("Could not seek to page");
  }
  file.read(page.data(), DiskPage::PAGE_SIZE);
  if (!file) {
    throw std::runtime_error("Page does not exist or could not be read");
  }
  return page;
}

void FileDiskManagerAdapter::writePage(DiskPageId pageId, DiskPage page) {
  std::fstream file(fileName, std::ios::in | std::ios::out | std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("Could not open existing database file");
  }

  const std::streamoff offset =
      static_cast<std::streamoff>(pageId) * DiskPage::PAGE_SIZE;
  file.seekp(offset);
  if (!file) {
    throw std::runtime_error("Could not seek to page");
  }
  file.write(page.data(), DiskPage::PAGE_SIZE);
  if (!file) {
    throw std::runtime_error("Could not write page");
  }
  file.flush();
  if (!file) {
    throw std::runtime_error("Could not flush page to disk");
  }
}
