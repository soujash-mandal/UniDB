#pragma once

#include "../../TupleService/actions/CreateTuplePageAction.h"
#include "../port/CreateTuplePagePort.h"

class SpaceMapCreateTuplePageAdapter : public CreateTuplePagePort {
public:
  explicit SpaceMapCreateTuplePageAdapter(
      CreateTuplePageAction &createTuplePageAction);

  CreateTuplePageResult createTuplePage() override;

private:
  CreateTuplePageAction &createTuplePageAction;
};
