#pragma once

#include <cstdint>

class CreateTuplePagePort {
public:
  virtual ~CreateTuplePagePort() = default;

  virtual uint32_t createTuplePage() = 0;
};
