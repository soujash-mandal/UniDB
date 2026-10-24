#pragma once
#include "../port/GetTablePagesPort.h"
class GetTablePagesAction;
class GetTablePagesAdapter final : public GetTablePagesPort {
public:
  explicit GetTablePagesAdapter(GetTablePagesAction &action);
  std::vector<uint32_t> getTablePages(uint32_t rootSpaceMapPageId) override;
private:
  GetTablePagesAction &action;
};
