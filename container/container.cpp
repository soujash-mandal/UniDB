#include "container.h"

Container::Container(DiskManagerPort &diskManager)
    : readPage(diskManager), writePage(diskManager),
      pageAdapter(readPage, writePage), createTuple(pageAdapter),
      readTuple(pageAdapter), updateTuple(pageAdapter),
      deleteTuple(pageAdapter), createPage(pageAdapter) {}

ReadPageAction &Container::readPageAction() { return readPage; }

WritePageAction &Container::writePageAction() { return writePage; }

CreateTupleAction &Container::createTupleAction() { return createTuple; }

ReadTupleAction &Container::readTupleAction() { return readTuple; }

UpdateTupleAction &Container::updateTupleAction() { return updateTuple; }

DeleteTupleAction &Container::deleteTupleAction() { return deleteTuple; }

CreatePageAction &Container::createPageAction() { return createPage; }