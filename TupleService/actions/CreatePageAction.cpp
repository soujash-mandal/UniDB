#include "CreatePageAction.h"

#include "../../core/PageHeader.h"

#include <cstring>

CreatePageAction::CreatePageAction(NewPagePort &newPagePort)
    : newPagePort(newPagePort) {}

void CreatePageAction::execute() {
  NewPageResult result = newPagePort.newPage();
  Page &page = result.page;
  PageHeader header{};
  header.pageId = result.pageId;
  header.slotCount = 0;
  header.freeSpaceOffset = Page::PAGE_SIZE;
  std::memcpy(page.data(), &header, sizeof(PageHeader));
}
