#include "container.h"

Container::Container(DiskPort &diskManager)
    : readPage(diskManager), writePage(diskManager),

      // FreePageManager
      readFreePageMetadataAdapter(readPage),
      writeFreePageMetadataAdapter(writePage),

      allocatePage(readFreePageMetadataAdapter, writeFreePageMetadataAdapter),
      freePage(readFreePageMetadataAdapter, writeFreePageMetadataAdapter),

      // BufferPoolManager
      readPageAdapter(readPage), writePageAdapter(writePage),

      // TupleService
      pageAdapter(readPage, writePage), createTuple(pageAdapter),
      getTuple(pageAdapter), deleteTuple(pageAdapter), createPage(pageAdapter) {
}

ReadPageAction &Container::readPageAction() { return readPage; }

WritePageAction &Container::writePageAction() { return writePage; }

CreateTupleAction &Container::createTupleAction() { return createTuple; }

GetTupleAction &Container::getTupleAction() { return getTuple; }

DeleteTupleAction &Container::deleteTupleAction() { return deleteTuple; }

CreatePageAction &Container::createPageAction() { return createPage; }