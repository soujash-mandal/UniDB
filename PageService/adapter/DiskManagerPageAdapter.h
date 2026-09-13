#pragma once
#include "../port/PagePort.h"

class DiskManagerPageAdapter : public PagePort {
public:
  explicit DiskManagerPageAdapter(PagePort &pagePort);
  void readPage(PageId pageId, Page &page) override;
  void writePage(PageId pageId, const Page &page) override;

private:
  PagePort &pagePort;
};