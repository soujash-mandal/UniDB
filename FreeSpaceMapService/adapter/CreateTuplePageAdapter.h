#pragma once

#include "../../TupleService/actions/CreateTuplePageAction.h"
#include "../port/CreateTuplePagePort.h"

class CreateTuplePageAdapter : public CreateTuplePagePort {
public:
  explicit CreateTuplePageAdapter(CreateTuplePageAction &createTuplePageAction);

  uint32_t createTuplePage() override;

private:
  CreateTuplePageAction &createTuplePageAction;
};
