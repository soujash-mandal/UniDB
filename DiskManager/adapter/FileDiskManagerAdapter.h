#pragma once

#include "../domain/DiskPage.h"
#include "../port/DiskPort.h"

#include <string>

class FileDiskManagerAdapter : public DiskPort {
public:
  // Configured by Container after dependency construction.
  std::string fileName;

  DiskPage readDiskPage(DiskPageId pageId) override;
  void writePage(DiskPageId pageId, DiskPage page) override;
};
