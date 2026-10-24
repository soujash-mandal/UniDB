#pragma once
#include "../../TupleService/actions/CreateTupleAction.h"
#include "../../TupleService/actions/CreateTuplePageAction.h"
#include "../../TupleService/actions/GetAllTuplesAction.h"
#include "../../TupleService/actions/GetTupleAction.h"
#include "../port/TuplePagePort.h"
class TuplePageAdapter final : public TuplePagePort {
public:
  TuplePageAdapter(CreateTuplePageAction &createTuplePageAction,
                   CreateTupleAction &createTupleAction,
                   GetTupleAction &getTupleAction,
                   GetAllTuplesAction &getAllTuplesAction);
  RdbCreatedTuplePage createTuplePage() override;
  uint32_t createTuple(uint32_t tuplePageId,
                       std::vector<char> tupleData) override;
  std::vector<char> getTuple(uint32_t tuplePageId,
                             uint32_t tupleSlotId) override;
  std::vector<std::vector<char>> getAllTuples(uint32_t tuplePageId) override;

private:
  CreateTuplePageAction &createTuplePageAction;
  CreateTupleAction &createTupleAction;
  GetTupleAction &getTupleAction;
  GetAllTuplesAction &getAllTuplesAction;
};