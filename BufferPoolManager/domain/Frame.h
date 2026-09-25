#pragma once

#include <cstdint>

#include "BufferPoolPage.h"
#include "BufferPoolPageId.h"

class Frame {
private:
  BufferPoolPage page;
  BufferPoolPageId pageId;
  uint32_t pinCount;
  bool dirty;
  bool occupied;

public:
  Frame() : pageId(0), pinCount(0), dirty(false), occupied(false) {}

  BufferPoolPage &getPage() { return page; }
  const BufferPoolPage &getPage() const { return page; }
  BufferPoolPageId getPageId() const { return pageId; }
  uint32_t getPinCount() const { return pinCount; }
  bool isDirty() const { return dirty; }
  bool isOccupied() const { return occupied; }
  void setPageId(BufferPoolPageId pageId) { this->pageId = pageId; }
  void setDirty(bool dirty) { this->dirty = dirty; }
  void setOccupied(bool occupied) { this->occupied = occupied; }
  void pin() { ++pinCount; }
  // todo: Frame::unpin() → decide whether an extra unpin should throw or
  // silently do nothing.
  void unpin() {
    if (pinCount > 0) {
      --pinCount;
    }
  }
};
