#pragma once

#include "../../TupleService/actions/CreateTuplePageAction.h"
#include "../port/CreateTuplePagePort.h"

class FSMCreateTuplePageAdapter : public CreateTuplePagePort {
public:
  explicit FSMCreateTuplePageAdapter(
      CreateTuplePageAction &createTuplePageAction);

  CreateTuplePageResult createTuplePage() override;

private:
  CreateTuplePageAction &createTuplePageAction;
};
