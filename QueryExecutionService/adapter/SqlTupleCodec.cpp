#include "SqlTupleCodec.h"

#include <cstring>
#include <stdexcept>
#include <type_traits>

namespace {
template <typename T> void append(std::vector<char> &out, const T &value) {
  const auto *begin = reinterpret_cast<const char *>(&value);
  out.insert(out.end(), begin, begin + sizeof(T));
}
template <typename T>
T read(const std::vector<char> &bytes, std::size_t &offset) {
  if (offset + sizeof(T) > bytes.size()) {
    throw std::runtime_error("Tuple bytes are truncated");
  }
  T value{};
  std::memcpy(&value, bytes.data() + offset, sizeof(T));
  offset += sizeof(T);
  return value;
}
} // namespace

std::vector<char>
SqlTupleCodec::encode(const std::vector<unidb::sql::ColumnDefinition> &schema,
                      const std::vector<unidb::sql::SqlValue> &values) {
  if (schema.size() != values.size()) {
    throw std::invalid_argument(
        "Tuple value count does not match table schema");
  }
  std::vector<char> out;
  for (std::size_t i = 0; i < schema.size(); ++i) {
    if (schema[i].type == unidb::sql::SqlType::Int) {
      if (!std::holds_alternative<std::int64_t>(values[i])) {
        throw std::invalid_argument("Expected INT value");
      }
      append(out, std::get<std::int64_t>(values[i]));
    } else if (schema[i].type == unidb::sql::SqlType::Text) {
      if (!std::holds_alternative<std::string>(values[i])) {
        throw std::invalid_argument("Expected TEXT value");
      }
      const auto &value = std::get<std::string>(values[i]);
      const uint32_t length = static_cast<uint32_t>(value.size());
      append(out, length);
      out.insert(out.end(), value.begin(), value.end());
    } else {
      throw std::invalid_argument(
          "V1 tuple codec supports only INT and VARCHAR/TEXT");
    }
  }
  return out;
}

std::vector<unidb::sql::SqlValue>
SqlTupleCodec::decode(const std::vector<Column> &schema,
                      const std::vector<char> &bytes) {
  std::size_t offset = 0;
  std::vector<unidb::sql::SqlValue> values;
  values.reserve(schema.size());
  for (const unidb::sql::ColumnDefinition &column : schema) {
    if (column.type == unidb::sql::SqlType::Int) {
      values.emplace_back(read<std::int64_t>(bytes, offset));
    } else if (column.type == unidb::sql::SqlType::Text) {
      const uint32_t length = read<uint32_t>(bytes, offset);
      if (offset + length > bytes.size()) {
        throw std::runtime_error("Tuple text value is truncated");
      }
      values.emplace_back(std::string(bytes.data() + offset, length));
      offset += length;
    } else {
      throw std::invalid_argument(
          "V1 tuple codec supports only INT and VARCHAR/TEXT");
    }
  }
  if (offset != bytes.size()) {
    throw std::runtime_error("Tuple bytes contain trailing data");
  }
  return values;
}
