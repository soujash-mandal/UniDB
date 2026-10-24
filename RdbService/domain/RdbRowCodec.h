#pragma once

#include "RdbTable.h"

#include <vector>

class RdbRowCodec {
public:
  static std::vector<char> encode(const RdbTableDefinition &table,
                                  const RdbRow &row);
  static RdbRow decode(const RdbTableDefinition &table,
                       const std::vector<char> &bytes);
};
