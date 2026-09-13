#include "CreateTupleAction.h"
#include <cstring>

CreateTupleAction::CreateTupleAction(PagePort &pagePort) : pagePort(pagePort) {}

void CreateTupleAction::execute(PageId pageId, const char *tupleData) {
  Page page;
  pagePort.readPage(pageId, page);
  std::memcpy(page.data(), tupleData, Page::PAGE_SIZE);
  pagePort.writePage(pageId, page);
}
