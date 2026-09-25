#pragma once

#include "../domain/DiskPage.h"
#include "../port/DiskPort.h"

#include <fstream>
#include <string>

class FileDiskManagerAdapter : public DiskPort {
public:
  explicit FileDiskManagerAdapter(const std::string &fileName);
  DiskPage readPage(DiskPageId pageId) override;
  void writePage(DiskPageId pageId, DiskPage &page) override;

private:
  std::fstream file;
};
