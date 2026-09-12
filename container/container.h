#pragma once

#include "../DiskManagerService/actions/ReadPageAction.h"
#include "../DiskManagerService/actions/WritePageAction.h"
#include "../DiskManagerService/port/DiskManagerPort.h"

class Container {
public:
  explicit Container(DiskManagerPort &diskManager);

  ReadPageAction &readPageAction();
  WritePageAction &writePageAction();

private:
  ReadPageAction readPage;
  WritePageAction writePage;
};