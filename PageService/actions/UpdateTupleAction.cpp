#include "UpdateTupleAction.h"

#include <cstring>

UpdateTupleAction::UpdateTupleAction(PagePort &pagePort) : pagePort(pagePort) {}

void UpdateTupleAction::execute(PageId pageId, const char *tupleData) {
  Page page;
  pagePort.readPage(pageId, page);
  std::memcpy(page.data(), tupleData, Page::PAGE_SIZE);
  pagePort.writePage(pageId, page);
}
