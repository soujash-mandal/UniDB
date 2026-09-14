#pragma once

#include "../DiskManagerService/actions/ReadPageAction.h"
#include "../DiskManagerService/actions/WritePageAction.h"
#include "../DiskManagerService/port/DiskManagerPort.h"

#include "../PageService/adapter/DiskManagerPageAdapter.h"

#include "../PageService/actions/CreatePageAction.h"
#include "../PageService/actions/CreateTupleAction.h"
#include "../PageService/actions/DeleteTupleAction.h"
#include "../PageService/actions/GetTupleAction.h"
#include "../PageService/actions/UpdateTupleAction.h"

class Container {

public:
  explicit Container(DiskManagerPort &diskManager);

  ReadPageAction &readPageAction();
  WritePageAction &writePageAction();

  CreateTupleAction &createTupleAction();
  GetTupleAction &getTupleAction();
  UpdateTupleAction &updateTupleAction();
  DeleteTupleAction &deleteTupleAction();
  CreatePageAction &createPageAction();

private:
  ReadPageAction readPage;
  WritePageAction writePage;

  DiskManagerPageAdapter pageAdapter;

  CreateTupleAction createTuple;
  GetTupleAction getTuple;
  UpdateTupleAction updateTuple;
  DeleteTupleAction deleteTuple;
  CreatePageAction createPage;
};