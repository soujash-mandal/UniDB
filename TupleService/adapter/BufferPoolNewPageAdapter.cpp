#include "BufferPoolNewPageAdapter.h"

BufferPoolNewPageAdapter::BufferPoolNewPageAdapter(NewPageAction &newPageAction)
    : newPageAction(newPageAction) {}

Page &BufferPoolNewPageAdapter::newPage() { return newPageAction.execute(); }
