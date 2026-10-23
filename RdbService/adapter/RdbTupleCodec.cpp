#include "RdbTupleCodec.h"

#include <bit>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace unidb::rdb {
namespace {

template <typename T> void appendLittleEndian(std::vector<char> &out, T value) {
  static_assert(std::is_integral_v<T>);
  using Unsigned = std::make_unsigned_t<T>;
  const auto raw = static_cast<Unsigned>(value);
  for (std::size_t i = 0; i < sizeof(Unsigned); ++i) {
    out.push_back(static_cast<char>((raw >> (i * 8U)) & 0xffU));
  }
}

template <typename T>
T readLittleEndian(const std::vector<char> &bytes, std::size_t &offset) {
  static_assert(std::is_integral_v<T>);
  using Unsigned = std::make_unsigned_t<T>;
  if (offset > bytes.size() || sizeof(Unsigned) > bytes.size() - offset) {
    throw std::runtime_error("Tuple bytes are truncated");
  }
  Unsigned raw = 0;
  for (std::size_t i = 0; i < sizeof(Unsigned); ++i) {
    raw |= static_cast<Unsigned>(static_cast<unsigned char>(bytes[offset + i]))
           << (i * 8U);
  }
  offset += sizeof(Unsigned);
  if constexpr (std::is_signed_v<T>) {
    return std::bit_cast<T>(raw);
  } else {
    return raw;
  }
}

} // namespace

std::vector<char>
RdbTupleCodec::encode(const std::vector<Column> &schema,
                      const std::vector<unidb::sql::SqlValue> &values) {
  if (schema.size() != values.size()) {
    throw std::invalid_argument(
        "Tuple value count does not match table schema");
  }

  std::vector<char> out;
  for (std::size_t i = 0; i < schema.size(); ++i) {
    if (schema[i].type == DataType::INT) {
      if (!std::holds_alternative<std::int64_t>(values[i])) {
        throw std::invalid_argument("Expected INT value");
      }
      appendLittleEndian(out, std::get<std::int64_t>(values[i]));
    } else if (schema[i].type == DataType::VARCHAR) {
      if (!std::holds_alternative<std::string>(values[i])) {
        throw std::invalid_argument("Expected TEXT value");
      }
      const auto &value = std::get<std::string>(values[i]);
      if (value.size() > std::numeric_limits<uint32_t>::max()) {
        throw std::invalid_argument("TEXT value exceeds V1 length limit");
      }
      appendLittleEndian(out, static_cast<uint32_t>(value.size()));
      out.insert(out.end(), value.begin(), value.end());
    } else {
      throw std::invalid_argument(
          "V1 tuple codec supports only INT and VARCHAR/TEXT");
    }
  }
  return out;
}

std::vector<unidb::sql::SqlValue>
RdbTupleCodec::decode(const std::vector<Column> &schema,
                      const std::vector<char> &bytes) {
  std::size_t offset = 0;
  std::vector<unidb::sql::SqlValue> values;
  values.reserve(schema.size());

  for (const Column &column : schema) {
    if (column.type == DataType::INT) {
      values.emplace_back(readLittleEndian<std::int64_t>(bytes, offset));
    } else if (column.type == DataType::VARCHAR) {
      const uint32_t length = readLittleEndian<uint32_t>(bytes, offset);
      if (offset > bytes.size() || length > bytes.size() - offset) {
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

} // namespace unidb::rdb
