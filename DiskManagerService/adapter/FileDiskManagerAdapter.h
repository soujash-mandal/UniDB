#pragma once

#include "../port/DiskManagerPort.h"

#include <fstream>
#include <string>

class FileDiskManagerAdapter : public DiskManagerPort {
public:
  explicit FileDiskManagerAdapter(const std::string &fileName);
  void writePage(PageId pageId, const Page &page) override;
  void readPage(PageId pageId, Page &page) override;

private:
  std::fstream file;
};
