#pragma once

#include <cstdint>
#include <stdexcept>

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

  BufferPoolPage getPage() { return page; }
  void setPage(BufferPoolPage page) { this->page = page; }

  BufferPoolPageId getPageId() const { return pageId; }
  void setPageId(BufferPoolPageId pageId) { this->pageId = pageId; }

  bool isDirty() const { return dirty; }
  void setDirty(bool dirty) { this->dirty = dirty; }

  bool isOccupied() const { return occupied; }
  void setOccupied(bool occupied) { this->occupied = occupied; }

  uint32_t getPinCount() const { return pinCount; }
  void pin() { ++pinCount; }
  void unpin() {
    if (pinCount == 0) {
      throw std::runtime_error("Cannot unpin page with pin count 0");
    }
    --pinCount;
  }
};
