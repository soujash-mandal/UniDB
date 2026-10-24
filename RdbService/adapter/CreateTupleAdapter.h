#pragma once
#include "../port/CreateTuplePort.h"
class CreateTupleAction;
class CreateTupleAdapter final : public CreateTuplePort {
public:
  explicit CreateTupleAdapter(CreateTupleAction &action);
  uint32_t createTuple(uint32_t tuplePageId, std::vector<char> tupleData) override;
private:
  CreateTupleAction &action;
};
