#pragma once
#include "../../core/TableId.h"
#include <string>

class Table {
public:
  Table(TableId id, const std::string &name) : id(id), name(name) {}
  TableId getId() const { return id; }
  const std::string &getName() const { return name; }

private:
  TableId id;
  std::string name;
};
