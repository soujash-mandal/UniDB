#pragma once

#include "../DiskManager/actions/ReadPageAction.h"
#include "../DiskManager/actions/WritePageAction.h"
#include "../DiskManager/port/DiskPort.h"

#include "../TupleService/adapter/DiskManagerPageAdapter.h"

#include "../TupleService/actions/CreatePageAction.h"
#include "../TupleService/actions/CreateTupleAction.h"
#include "../TupleService/actions/DeleteTupleAction.h"
#include "../TupleService/actions/GetTupleAction.h"

class Container {

public:
  explicit Container(DiskPort &diskManager);

  ReadPageAction &readPageAction();
  WritePageAction &writePageAction();

  CreateTupleAction &createTupleAction();
  GetTupleAction &getTupleAction();
  DeleteTupleAction &deleteTupleAction();
  CreatePageAction &createPageAction();

private:
  ReadPageAction readPage;
  WritePageAction writePage;

  DiskManagerPageAdapter pageAdapter;

  CreateTupleAction createTuple;
  GetTupleAction getTuple;
  DeleteTupleAction deleteTuple;
  CreatePageAction createPage;
};