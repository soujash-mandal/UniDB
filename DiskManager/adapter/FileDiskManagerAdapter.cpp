#include "FileDiskManagerAdapter.h"

#include <stdexcept>

FileDiskManagerAdapter::FileDiskManagerAdapter(const std::string &fileName) {
  file.open(fileName, std::ios::in | std::ios::out | std::ios::binary);
  if (!file.is_open()) {
    file.clear();
    file.open(fileName, std::ios::out | std::ios::binary);
    file.close();
    file.open(fileName, std::ios::in | std::ios::out | std::ios::binary);
  }
  if (!file.is_open()) {
    throw std::runtime_error("Could not open database file");
  }
}

DiskPage FileDiskManagerAdapter::readPage(DiskPageId pageId) {
  DiskPage page;
  const std::streamoff offset =
      static_cast<std::streamoff>(pageId) * DiskPage::PAGE_SIZE;
  file.seekg(offset);
  if (!file) {
    throw std::runtime_error("Page does not exist");
  }
  file.read(page.data(), DiskPage::PAGE_SIZE);
  if (!file) {
    throw std::runtime_error("Could not read page");
  }
  return page;
}

void FileDiskManagerAdapter::writePage(DiskPageId pageId, DiskPage &page) {
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
}
