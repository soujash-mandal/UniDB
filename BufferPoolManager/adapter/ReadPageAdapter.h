#pragma once

#include "../../DiskManager/actions/ReadPageAction.h"
#include "../port/ReadPagePort.h"

class ReadPageAdapter : public ReadPagePort {
public:
  explicit ReadPageAdapter(ReadPageAction &readPageAction);

  void readPage(PageId pageId, Page &page) override;

private:
  ReadPageAction &readPageAction;
};
