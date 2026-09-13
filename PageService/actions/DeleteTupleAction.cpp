#include "DeleteTupleAction.h"

#include <cstring>

DeleteTupleAction::DeleteTupleAction(PagePort &pagePort) : pagePort(pagePort) {}

void DeleteTupleAction::execute(PageId pageId) {
  Page page;
  pagePort.readPage(pageId, page);
  std::memset(page.data(), 0, Page::PAGE_SIZE);
  pagePort.writePage(pageId, page);
}
