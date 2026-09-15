#include "CreatePageAction.h"
#include "../../core/PageHeader.h"
#include <cstring>

CreatePageAction::CreatePageAction(PagePort &pagePort) : pagePort(pagePort) {}

void CreatePageAction::execute(const PageId &pageId) {

  Page page;
  PageHeader header{};
  header.pageId = pageId;
  header.slotCount = 0;
  header.freeSpaceOffset = Page::PAGE_SIZE;

  std::memcpy(page.data(), &header, sizeof(PageHeader));

  pagePort.writePage(pageId, page);
}
