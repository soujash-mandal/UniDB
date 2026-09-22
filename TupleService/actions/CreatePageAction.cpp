#include "CreatePageAction.h"

#include "../../core/PageHeader.h"

#include <cstring>

CreatePageAction::CreatePageAction(NewPagePort &newPagePort)
    : newPagePort(newPagePort) {}

void CreatePageAction::execute(const PageId &pageId) {

  Page &page = newPagePort.newPage();
  PageHeader header{};
  header.pageId = pageId;
  header.slotCount = 0;
  header.freeSpaceOffset = Page::PAGE_SIZE;
  std::memcpy(page.data(), &header, sizeof(PageHeader));
}
