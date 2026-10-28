#pragma once
#include "../../SpaceMapService/domain/SpaceMapPage.h"
#include "../../SpaceMapService/port/CreateTuplePagePort.h"
#include "../../SpaceMapService/port/FetchPagePort.h"
#include "../../SpaceMapService/port/NewPagePort.h"
#include "../../SpaceMapService/port/UnpinPagePort.h"
#include "../../SpaceMapService/port/WritePagePort.h"
#include "../../TupleService/domain/TuplePage.h"
#include "../../TupleService/port/FetchPagePort.h"
#include "../../TupleService/port/NewPagePort.h"
#include "../../TupleService/port/UnpinPagePort.h"
#include "../../TupleService/port/WritePagePort.h"
#include <map>
#include <stdexcept>
#include <vector>

class FakeSpaceMapStore final : public NewPagePort,
                                public SpaceMapWritePagePort,
                                public UnpinPagePort,
                                public FetchPagePort,
                                public CreateTuplePagePort {
public:
  SpaceMapPageId newPage() override {
    const auto id = nextPageId++;
    pages.emplace(id, SpaceMapPage{});
    return id;
  }
  void writePage(SpaceMapPageId id, SpaceMapPage page) override {
    pages[id] = page;
  }
  void unpinPage(SpaceMapPageId id) override { unpinned.push_back(id); }
  SpaceMapPage fetchPage(SpaceMapPageId id) override {
    const auto it = pages.find(id);
    if (it == pages.end())
      throw std::runtime_error("SpaceMap page not found");
    return it->second;
  }
  CreateTuplePageResult createTuplePage() override {
    return {nextTuplePageId++, 8000};
  }
  std::map<SpaceMapPageId, SpaceMapPage> pages;
  std::vector<SpaceMapPageId> unpinned;

private:
  SpaceMapPageId nextPageId = 20;
  TuplePageId nextTuplePageId = 200;
};

class FakeTupleStore final : public TupleNewPagePort,
                             public TupleWritePagePort,
                             public TupleUnpinPagePort,
                             public TupleFetchPagePort {
public:
  TuplePageId newPage() override {
    const auto id = nextPageId++;
    pages.emplace(id, TuplePage{});
    return id;
  }
  void writePage(TuplePageId id, TuplePage page) override { pages[id] = page; }
  void unpinPage(TuplePageId id) override { unpinned.push_back(id); }
  TuplePage fetchPage(TuplePageId id) override {
    const auto it = pages.find(id);
    if (it == pages.end())
      throw std::runtime_error("Tuple page not found");
    return it->second;
  }
  std::map<TuplePageId, TuplePage> pages;
  std::vector<TuplePageId> unpinned;

private:
  TuplePageId nextPageId = 50;
};
