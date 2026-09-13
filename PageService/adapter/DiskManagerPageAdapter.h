#pragma once
#include "../port/PagePort.h"

#include "../../DiskManagerService/actions/ReadPageAction.h"
#include "../../DiskManagerService/actions/WritePageAction.h"

class DiskManagerPageAdapter : public PagePort {
public:
  explicit DiskManagerPageAdapter(ReadPageAction &readPage,
                                  WritePageAction &writePage);

  void readPage(PageId pageId, Page &page) override;
  void writePage(PageId pageId, const Page &page) override;

private:
  ReadPageAction &readPageAction;
  WritePageAction &writePageAction;
};