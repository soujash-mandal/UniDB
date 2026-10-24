#pragma once

#include "../port/SpaceMapPort.h"

class CreateSpaceMapPageAction;
class FindPageWithSpaceAction;
class GetTablePagesAction;

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
