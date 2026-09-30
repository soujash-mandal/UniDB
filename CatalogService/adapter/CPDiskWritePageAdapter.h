#pragma once

#include "../../DiskManager/actions/WriteDiskPageAction.h"
#include "../port/DiskWritePagePort.h"

class CPDiskWritePageAdapter : public DiskWritePagePort {
public:
  explicit CPDiskWritePageAdapter(WriteDiskPageAction &writePageAction);
  void writePage(CatalogPageId pageId, CatalogPage page) override;

private:
  WriteDiskPageAction &writePageAction;
};
