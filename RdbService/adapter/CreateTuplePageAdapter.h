#pragma once
#include "../port/CreateTuplePagePort.h"
class CreateTuplePageAction;
class CreateTuplePageAdapter final : public RdbCreateTuplePagePort {
public:
  explicit CreateTuplePageAdapter(CreateTuplePageAction &action);
  RdbCreatedTuplePage createTuplePage() override;

private:
  CreateTuplePageAction &action;
};
