#pragma once

#include "../domain/DiskPage.h"
#include "../port/DiskPort.h"

#include <fstream>
#include <string>

class FileDiskManagerAdapter : public DiskPort {
public:
  explicit FileDiskManagerAdapter(const std::string &fileName);
  DiskPage readDiskPage(DiskPageId pageId) override;
  void writePage(DiskPageId pageId, DiskPage page) override;

private:
  std::fstream file;
};
