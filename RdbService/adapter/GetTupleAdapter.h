#pragma once
#include "../port/GetTuplePort.h"
class GetTupleAction;
class GetTupleAdapter final : public GetTuplePort {
public:
  explicit GetTupleAdapter(GetTupleAction &action);
  std::vector<char> getTuple(uint32_t tuplePageId, uint32_t tupleSlotId) override;
private:
  GetTupleAction &action;
};
