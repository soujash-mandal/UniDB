#include "container.h"

Container::Container(DiskManagerPort &diskManager)
    : readPage(diskManager), writePage(diskManager) {}

ReadPageAction &Container::readPageAction() { return readPage; }
WritePageAction &Container::writePageAction() { return writePage; }