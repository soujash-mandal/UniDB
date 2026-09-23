#include "FSMBufferPoolUnpinPageAdapter.h"

FSMBufferPoolUnpinPageAdapter::FSMBufferPoolUnpinPageAdapter(
    UnpinPageAction &unpinPageAction)
    : unpinPageAction(unpinPageAction) {}

void FSMBufferPoolUnpinPageAdapter::unpinPage(PageId pageId, bool dirty) {
  unpinPageAction.execute(pageId, dirty);
}
