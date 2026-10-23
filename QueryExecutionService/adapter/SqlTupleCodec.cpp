#include "SqlTupleCodec.h"

#include <bit>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace {
template <typename T> void appendLittleEndian(std::vector<char> &out, T value) {
  static_assert(std::is_integral_v<T>);
  using Unsigned = std::make_unsigned_t<T>;
  const auto raw = static_cast<Unsigned>(value);
  for (std::size_t i = 0; i < sizeof(T); ++i) {
    out.push_back(static_cast<char>((raw >> (i * 8U)) & Unsigned{0xff}));
  }
}

template <typename T>
T readLittleEndian(const std::vector<char> &bytes, std::size_t &offset) {
  static_assert(std::is_integral_v<T>);
  if (offset > bytes.size() || sizeof(T) > bytes.size() - offset) {
    throw std::runtime_error("Tuple bytes are truncated");
  }

  using Unsigned = std::make_unsigned_t<T>;
  Unsigned raw{};
  for (std::size_t i = 0; i < sizeof(T); ++i) {
    raw |= static_cast<Unsigned>(
               static_cast<unsigned char>(bytes[offset + i]))
           << (i * 8U);
  }
  offset += sizeof(T);
  if constexpr (std::is_signed_v<T>) {
    return std::bit_cast<T>(raw);
  } else {
    return raw;
  }
}
} // namespace

std::vector<char>
SqlTupleCodec::encode(const std::vector<unidb::sql::SqlType> &schema,
                      const std::vector<unidb::sql::SqlValue> &values) {
  if (schema.size() != values.size()) {
    throw std::invalid_argument(
        "Tuple value count does not match table schema");
  }
  std::vector<char> out;
  for (std::size_t i = 0; i < schema.size(); ++i) {
    if (schema[i] == unidb::sql::SqlType::Int) {
      if (!std::holds_alternative<std::int64_t>(values[i])) {
        throw std::invalid_argument("Expected INT value");
      }
      appendLittleEndian(out, std::get<std::int64_t>(values[i]));
    } else if (schema[i] == unidb::sql::SqlType::Text) {
      if (!std::holds_alternative<std::string>(values[i])) {
        throw std::invalid_argument("Expected TEXT value");
      }
      const auto &value = std::get<std::string>(values[i]);
      if (value.size() > std::numeric_limits<uint32_t>::max()) {
        throw std::invalid_argument("TEXT value exceeds V1 tuple length limit");
      }
      const auto length = static_cast<uint32_t>(value.size());
      appendLittleEndian(out, length);
      out.insert(out.end(), value.begin(), value.end());
    } else {
      throw std::invalid_argument("V1 tuple codec supports only INT and TEXT");
    }
  }
  return out;
}

std::vector<unidb::sql::SqlValue>
SqlTupleCodec::decode(const std::vector<unidb::sql::SqlType> &schema,
                      const std::vector<char> &bytes) {
  std::size_t offset = 0;
  std::vector<unidb::sql::SqlValue> values;
  values.reserve(schema.size());
  for (const auto type : schema) {
    if (type == unidb::sql::SqlType::Int) {
      values.emplace_back(readLittleEndian<std::int64_t>(bytes, offset));
    } else if (type == unidb::sql::SqlType::Text) {
      const uint32_t length = readLittleEndian<uint32_t>(bytes, offset);
      if (offset > bytes.size() || length > bytes.size() - offset) {
        throw std::runtime_error("Tuple text value is truncated");
      }
      values.emplace_back(std::string(bytes.data() + offset, length));
      offset += length;
    } else {
      throw std::invalid_argument("V1 tuple codec supports only INT and TEXT");
    }
  }
  if (offset != bytes.size()) {
    throw std::runtime_error("Tuple bytes contain trailing data");
  }
  return values;
}
