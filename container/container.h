#pragma once

#include "../DiskManagerService/actions/ReadPageAction.h"
#include "../DiskManagerService/actions/WritePageAction.h"
#include "../DiskManagerService/port/DiskManagerPort.h"

#include "../PageService/adapter/DiskManagerPageAdapter.h"

#include "../PageService/actions/CreateTupleAction.h"
#include "../PageService/actions/DeleteTupleAction.h"
#include "../PageService/actions/ReadTupleAction.h"
#include "../PageService/actions/UpdateTupleAction.h"

class Container {

public:
  explicit Container(DiskManagerPort &diskManager);

  ReadPageAction &readPageAction();
  WritePageAction &writePageAction();

  CreateTupleAction &createTupleAction();
  ReadTupleAction &readTupleAction();
  UpdateTupleAction &updateTupleAction();
  DeleteTupleAction &deleteTupleAction();

private:
  ReadPageAction readPage;
  WritePageAction writePage;

  DiskManagerPageAdapter pageAdapter;

  CreateTupleAction createTuple;
  ReadTupleAction readTuple;
  UpdateTupleAction updateTuple;
  DeleteTupleAction deleteTuple;
};