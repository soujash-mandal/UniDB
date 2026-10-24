#pragma once

#include "../../actions/GetAllTuplesAction.h"

#include <stdexcept>

class GetAllTuplesFetchPort final : public TupleFetchPagePort {
public:
  TuplePage page;
  bool failFetch = false;

  TuplePage fetchPage(TuplePageId) override {
    if (failFetch) {
      throw std::runtime_error("fetch failed");
    }
    return page;
  }
};

class GetAllTuplesUnpinPort final : public TupleUnpinPagePort {
public:
  int calls = 0;
  TuplePageId lastPageId = 0;
  bool failUnpin = false;

  void unpinPage(TuplePageId pageId) override {
    ++calls;
    lastPageId = pageId;
    if (failUnpin) {
      throw std::runtime_error("unpin failed");
    }
  }
};
