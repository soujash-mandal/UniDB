#include "container.h"

Container::Container(DiskPort &diskManager)
    : readPage(diskManager), writePage(diskManager),
      pageAdapter(readPage, writePage), createTuple(pageAdapter),
      getTuple(pageAdapter), deleteTuple(pageAdapter), createPage(pageAdapter) {
}

ReadPageAction &Container::readPageAction() { return readPage; }

WritePageAction &Container::writePageAction() { return writePage; }

CreateTupleAction &Container::createTupleAction() { return createTuple; }

GetTupleAction &Container::getTupleAction() { return getTuple; }

DeleteTupleAction &Container::deleteTupleAction() { return deleteTuple; }

CreatePageAction &Container::createPageAction() { return createPage; }