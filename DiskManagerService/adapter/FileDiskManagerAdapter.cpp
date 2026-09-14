#include "FileDiskManagerAdapter.h"
#include "../../core/exceptions/PageNotFoundException.h"
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

void FileDiskManagerAdapter::writePage(const PageId &pageId, const Page &page) {
  const std::streamoff offset =
      static_cast<std::streamoff>(pageId) * Page::PAGE_SIZE;
  file.seekp(offset);
  file.write(page.data(), Page::PAGE_SIZE);
  file.flush();
}

void FileDiskManagerAdapter::readPage(const PageId &pageId, Page &page) {

  const std::streamoff offset =
      static_cast<std::streamoff>(pageId) * Page::PAGE_SIZE;
  file.seekg(offset);
  if (!file) {
    throw PageNotFoundException("Page does not exist");
  }
  file.read(page.data(), Page::PAGE_SIZE);
  if (!file) {
    throw std::runtime_error("Could not read page");
  }
}