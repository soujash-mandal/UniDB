#pragma once

#include "../DiskManagerService/actions/ReadPageAction.h"
#include "../DiskManagerService/actions/WritePageAction.h"
#include "../DiskManagerService/adapter/FileDiskManagerAdapter.h"

class Container {
public:
  explicit Container(const std::string &databaseFile);
  ReadPageAction &readPageAction();
  WritePageAction &writePageAction();

private:
  FileDiskManagerAdapter diskManagerAdapter;
  ReadPageAction readPage;
  WritePageAction writePage;
};