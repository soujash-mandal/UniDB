#pragma once

#include <cstdint>
#include <string>

#include "DataType.h"

struct Column {
  std::string name;
  DataType type;
  uint16_t size = 0;
  bool nullable = false;
};
