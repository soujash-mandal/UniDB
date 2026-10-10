#include "FileDiskManagerAdapter.h"

#include <stdexcept>

void FileDiskManagerAdapter::setFileName(const std::string &name) {
  if (file.is_open()) {
    file.close();
  }
  fileName = name;
}

void FileDiskManagerAdapter::openExistingFile() {
  if (file.is_open()) {
    return;
  }
  if (fileName.empty()) {
    throw std::runtime_error("Database file name has not been configured");
  }

  file.open(fileName, std::ios::in | std::ios::out | std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("Could not open existing database file");
  }
}

DiskPage FileDiskManagerAdapter::readDiskPage(DiskPageId pageId) {
  openExistingFile();
  file.clear();

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
  openExistingFile();
  file.clear();

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
