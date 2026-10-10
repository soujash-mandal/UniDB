#pragma once

#include "../domain/DiskPage.h"
#include "../port/DiskPort.h"

#include <fstream>
#include <string>

class FileDiskManagerAdapter : public DiskPort {
public:
  void setFileName(const std::string &fileName);
  DiskPage readDiskPage(DiskPageId pageId) override;
  void writePage(DiskPageId pageId, DiskPage page) override;

private:
  void openExistingFile();

  std::string fileName;
  std::fstream file;
};
