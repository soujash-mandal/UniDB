#pragma once
#include "../port/GetAllTuplesPort.h"
class GetAllTuplesAction;
class GetAllTuplesAdapter final : public GetAllTuplesPort {
public:
  explicit GetAllTuplesAdapter(GetAllTuplesAction &action);
  std::vector<std::vector<char>> getAllTuples(uint32_t tuplePageId) override;

private:
  GetAllTuplesAction &action;
};
