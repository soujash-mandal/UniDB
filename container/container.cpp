#include "container.h"

Container::Container(const std::string &databaseFile)
    : diskManagerAdapter(databaseFile), readPage(diskManagerAdapter),
      writePage(diskManagerAdapter) {}

ReadPageAction &Container::readPageAction() { return readPage; }
WritePageAction &Container::writePageAction() { return writePage; }