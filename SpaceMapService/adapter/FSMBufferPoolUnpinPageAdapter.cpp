#include "FSMBufferPoolUnpinPageAdapter.h"

FSMBufferPoolUnpinPageAdapter::FSMBufferPoolUnpinPageAdapter(
    UnpinPageAction &unpinPageAction)
    : unpinPageAction(unpinPageAction) {}

void FSMBufferPoolUnpinPageAdapter::unpinPage(FSMPageId pageId) {
  unpinPageAction.execute(pageId);
}
