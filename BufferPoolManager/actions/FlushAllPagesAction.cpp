#include "FlushAllPagesAction.h"

FlushAllPagesAction::FlushAllPagesAction(BufferPool &bufferPool,
                                         WritePagePort &writePagePort)
    : bufferPool(bufferPool), writePagePort(writePagePort) {}

void FlushAllPagesAction::execute() {
  for (uint32_t frameId = 0; frameId < bufferPool.size(); ++frameId) {
    Frame &frame = bufferPool.getFrame(frameId);
    if (!frame.isOccupied() || !frame.isDirty()) {
      continue;
    }
    writePagePort.writePage(frame.getPageId(), frame.getPage());
    frame.setDirty(false);
  }
}
