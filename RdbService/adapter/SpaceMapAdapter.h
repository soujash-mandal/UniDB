#pragma once
#include "../../SpaceMapService/actions/CreateSpaceMapPageAction.h"
#include "../../SpaceMapService/actions/FindPageWithSpaceAction.h"
#include "../../SpaceMapService/actions/GetTablePagesAction.h"
#include "../port/SpaceMapPort.h"
class SpaceMapAdapter final : public SpaceMapPort {
public:
  SpaceMapAdapter(CreateSpaceMapPageAction &createSpaceMapPageAction,
                  FindPageWithSpaceAction &findPageWithSpaceAction,
                  GetTablePagesAction &getTablePagesAction);
  uint32_t createSpaceMapPage() override;
  RdbFindPageWithSpaceResult findPageWithSpace(
      uint32_t rootSpaceMapPageId, uint32_t requiredSpace) override;
  std::vector<uint32_t> getTablePages(uint32_t rootSpaceMapPageId) override;
private:
  CreateSpaceMapPageAction &createSpaceMapPageAction;
  FindPageWithSpaceAction &findPageWithSpaceAction;
  GetTablePagesAction &getTablePagesAction;
};