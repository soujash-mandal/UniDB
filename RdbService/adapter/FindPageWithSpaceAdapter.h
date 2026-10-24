#pragma once
#include "../port/FindPageWithSpacePort.h"
class FindPageWithSpaceAction;
class FindPageWithSpaceAdapter final : public FindPageWithSpacePort {
public:
  explicit FindPageWithSpaceAdapter(FindPageWithSpaceAction &action);
  RdbFindPageWithSpaceResult findPageWithSpace(uint32_t rootSpaceMapPageId,
                                               uint32_t requiredSpace) override;

private:
  FindPageWithSpaceAction &action;
};
