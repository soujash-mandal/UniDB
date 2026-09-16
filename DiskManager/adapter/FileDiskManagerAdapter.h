#pragma once

#include "../port/DiskPort.h"

#include <fstream>
#include <string>

class FileDiskManagerAdapter : public DiskPort {
public:
  explicit FileDiskManagerAdapter(const std::string &fileName);
  void writePage(const PageId &pageId, const Page &page) override;
  void readPage(const PageId &pageId, Page &page) override;

private:
  std::fstream file;
};
