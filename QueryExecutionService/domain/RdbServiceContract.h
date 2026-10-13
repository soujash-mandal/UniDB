#pragma once

#include "../../SQLParserContract/sqlStatement.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <variant>
#include <vector>

// QueryExecutionService owns these contracts; the adapter maps RDBService results.
namespace unidb::query {
enum class RdbServiceErrorCode {
  InvalidArgument,
  TableNotFound,
  DuplicateTable,
  UnknownColumn,
  TypeMismatch,
  UnsupportedType,
  StorageFailure,
  InternalError
};

struct RdbServiceError {
  RdbServiceErrorCode code;
  std::string message;
};

template <typename T> class RdbServiceResult {
public:
  static RdbServiceResult success(T value) {
    return RdbServiceResult(
        std::variant<T, RdbServiceError>(std::in_place_index<0>,
                                         std::move(value)));
  }

  static RdbServiceResult failure(RdbServiceError error) {
    return RdbServiceResult(
        std::variant<T, RdbServiceError>(std::in_place_index<1>,
                                         std::move(error)));
  }

  bool hasValue() const { return result.index() == 0; }
  explicit operator bool() const { return hasValue(); }
  const T &value() const { return std::get<0>(result); }
  T &value() { return std::get<0>(result); }
  const RdbServiceError &error() const { return std::get<1>(result); }

private:
  explicit RdbServiceResult(std::variant<T, RdbServiceError> result)
      : result(std::move(result)) {}
  std::variant<T, RdbServiceError> result;
};

using RdbServiceStatus = RdbServiceResult<std::monostate>;

struct QueryTableDefinition {
  uint32_t tableId{};
  std::string tableName;
  std::vector<unidb::sql::ColumnDefinition> columns;
};
} // namespace unidb::query
