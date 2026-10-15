#pragma once

#include "../domain/RdbServiceContract.h"

#include <cstddef>
#include <string>
#include <vector>

namespace unidb::query {
class RdbServicePort {
public:
  virtual ~RdbServicePort() = default;

  virtual RdbServiceResult<uint32_t>
  createTable(const unidb::sql::CreateTableStatement &) = 0;
  virtual RdbServiceStatus dropTable(const std::string &) = 0;
  virtual RdbServiceResult<QueryTableDefinition>
  getTableDefinition(const std::string &) = 0;
  virtual RdbServiceResult<std::size_t>
  insertRow(const unidb::sql::InsertStatement &) = 0;
  virtual RdbServiceResult<std::vector<std::vector<unidb::sql::SqlValue>>>
  selectRows(const unidb::sql::SelectStatement &) = 0;
  virtual RdbServiceResult<std::size_t>
  deleteRows(const std::string &, const unidb::sql::WhereEquals &) = 0;
};
} // namespace unidb::query
