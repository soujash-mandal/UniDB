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

void FileDiskManagerAdapter::writePage(PageId pageId, const Page &page) {
  const std::streamoff offset =
      static_cast<std::streamoff>(pageId) * Page::PAGE_SIZE;
  file.seekp(offset);
  file.write(page.data(), Page::PAGE_SIZE);
  file.flush();
}

void FileDiskManagerAdapter::readPage(PageId pageId, Page &page) {
  const std::streamoff offset =
      static_cast<std::streamoff>(pageId) * Page::PAGE_SIZE;
  file.seekg(offset);
  file.read(page.data(), Page::PAGE_SIZE);
}