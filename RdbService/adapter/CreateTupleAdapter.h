#pragma once
#include "../port/CreateTuplePort.h"
class CreateTupleAction;
class CreateTupleAdapter final : public CreateTuplePort {
public:
  explicit CreateTupleAdapter(CreateTupleAction &action);
  CreateTupleResult createTuple(uint32_t tuplePageId,
                                std::vector<char> tupleData) override;

private:
  CreateTupleAction &action;
};
