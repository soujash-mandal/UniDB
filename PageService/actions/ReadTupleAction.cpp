#include "ReadTupleAction.h"

ReadTupleAction::ReadTupleAction(PagePort &pagePort) : pagePort(pagePort) {}

void ReadTupleAction::execute(PageId pageId, Page &page) {
  pagePort.readPage(pageId, page);
}
