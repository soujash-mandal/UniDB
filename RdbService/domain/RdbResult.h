#pragma once
#include <exception>
#include <stdexcept>
#include <string>
#include <variant>
#include <utility>

namespace unidb::rdb {
enum class RdbErrorCode {
  InvalidArgument,
  TableNotFound,
  DuplicateTable,
  UnknownColumn,
  TypeMismatch,
  UnsupportedType,
  StorageFailure,
  InternalError
};
struct RdbError {
  RdbErrorCode code;
  std::string message;
};
template <typename T> class RdbResult {
public:
  static RdbResult success(T value) { return RdbResult(std::variant<T, RdbError>(std::in_place_index<0>, std::move(value))); }
  static RdbResult failure(RdbError error) { return RdbResult(std::variant<T, RdbError>(std::in_place_index<1>, std::move(error))); }
  bool hasValue() const { return result.index() == 0; }
  explicit operator bool() const { return hasValue(); }
  const T &value() const { return std::get<0>(result); }
  T &value() { return std::get<0>(result); }
  const RdbError &error() const { return std::get<1>(result); }
private:
  explicit RdbResult(std::variant<T, RdbError> result) : result(std::move(result)) {}
  std::variant<T, RdbError> result;
};
using RdbStatus = RdbResult<std::monostate>;
inline RdbError classifyError(const std::exception &exception) {
  const std::string message = exception.what();
  if (message.find("Table not found") != std::string::npos)
    return {RdbErrorCode::TableNotFound, message};
  if (message.find("already exists") != std::string::npos)
    return {RdbErrorCode::DuplicateTable, message};
  if (message.find("column") != std::string::npos || message.find("Column") != std::string::npos)
    return {RdbErrorCode::UnknownColumn, message};
  if (message.find("Expected INT value") != std::string::npos || message.find("Expected TEXT value") != std::string::npos)
    return {RdbErrorCode::TypeMismatch, message};
  if (dynamic_cast<const std::invalid_argument *>(&exception) != nullptr)
    return {RdbErrorCode::InvalidArgument, message};
  if (message.find("supports only") != std::string::npos || message.find("Unsupported") != std::string::npos)
    return {RdbErrorCode::UnsupportedType, message};
  return {RdbErrorCode::StorageFailure, message};
}
}