#include "SqlTupleCodec.h"

#include "../types/SqlIntegerSemantics.h"

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
    raw |= static_cast<Unsigned>(static_cast<unsigned char>(bytes[offset + i]))
           << (i * 8U);
  }
  offset += sizeof(T);
  if constexpr (std::is_signed_v<T>) {
    return std::bit_cast<T>(raw);
  } else {
    return raw;
  }
}

void appendText(std::vector<char> &out, const std::string &value) {
  if (value.size() > std::numeric_limits<uint32_t>::max()) {
    throw std::invalid_argument("TEXT value exceeds V1 tuple length limit");
  }
  appendLittleEndian(out, static_cast<uint32_t>(value.size()));
  out.insert(out.end(), value.begin(), value.end());
}

std::string readText(const std::vector<char> &bytes, std::size_t &offset) {
  const uint32_t length = readLittleEndian<uint32_t>(bytes, offset);
  if (offset > bytes.size() || length > bytes.size() - offset) {
    throw std::runtime_error("Tuple text value is truncated");
  }
  std::string value(bytes.data() + offset, length);
  offset += length;
  return value;
}

bool usesDeclaredRowFormat(
    const std::vector<unidb::sql::ColumnDefinition> &schema) {
  bool hasDeclaredType = false;
  bool everyColumnHasDeclaredType = !schema.empty();
  for (const auto &column : schema) {
    hasDeclaredType = hasDeclaredType || column.declaredType.has_value();
    everyColumnHasDeclaredType =
        everyColumnHasDeclaredType && column.declaredType.has_value();
  }
  if (hasDeclaredType && !everyColumnHasDeclaredType) {
    throw std::invalid_argument(
        "Tuple schema cannot mix declared and transitional column types");
  }
  return everyColumnHasDeclaredType;
}

void appendDeclaredInteger(std::vector<char> &out, unidb::sql::SqlTypeId type,
                           std::int64_t value) {
  const auto checked =
      QueryExecutionService::domain::makeSqlIntegerValue(type, value);
  std::visit([&out](const auto integer) { appendLittleEndian(out, integer); },
             checked);
}

std::int64_t readDeclaredInteger(unidb::sql::SqlTypeId type,
                                 const std::vector<char> &bytes,
                                 std::size_t &offset) {
  using unidb::sql::SqlTypeId;
  switch (type) {
  case SqlTypeId::SmallInt:
    return readLittleEndian<std::int16_t>(bytes, offset);
  case SqlTypeId::Integer:
    return readLittleEndian<std::int32_t>(bytes, offset);
  case SqlTypeId::BigInt:
    return readLittleEndian<std::int64_t>(bytes, offset);
  default:
    throw std::invalid_argument("SQL type is not an integer type");
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
      appendText(out, std::get<std::string>(values[i]));
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
      values.emplace_back(readText(bytes, offset));
    } else {
      throw std::invalid_argument("V1 tuple codec supports only INT and TEXT");
    }
  }
  if (offset != bytes.size()) {
    throw std::runtime_error("Tuple bytes contain trailing data");
  }
  return values;
}

std::vector<char>
SqlTupleCodec::encode(const std::vector<unidb::sql::ColumnDefinition> &schema,
                      const std::vector<unidb::sql::SqlValue> &values) {
  constexpr std::uint8_t rowFormatVersion = 1;
  const bool versioned = usesDeclaredRowFormat(schema);
  if (schema.size() != values.size()) {
    throw std::invalid_argument(
        "Tuple value count does not match table schema");
  }

  std::vector<char> out;
  if (versioned) {
    appendLittleEndian(out, rowFormatVersion);
  }
  for (std::size_t index = 0; index < schema.size(); ++index) {
    const auto &column = schema[index];
    if (!column.declaredType.has_value()) {
      if (column.type == unidb::sql::SqlType::Int) {
        if (!std::holds_alternative<std::int64_t>(values[index])) {
          throw std::invalid_argument("Expected INT value");
        }
        appendLittleEndian(out, std::get<std::int64_t>(values[index]));
      } else if (column.type == unidb::sql::SqlType::Text) {
        if (!std::holds_alternative<std::string>(values[index])) {
          throw std::invalid_argument("Expected TEXT value");
        }
        appendText(out, std::get<std::string>(values[index]));
      } else {
        throw std::invalid_argument("Unsupported SQL tuple column type");
      }
      continue;
    }

    using unidb::sql::SqlTypeId;
    switch (column.declaredType->id) {
    case SqlTypeId::SmallInt:
    case SqlTypeId::Integer:
    case SqlTypeId::BigInt:
      if (!std::holds_alternative<std::int64_t>(values[index])) {
        throw std::invalid_argument("Expected integer value");
      }
      appendDeclaredInteger(out, column.declaredType->id,
                            std::get<std::int64_t>(values[index]));
      break;
    case SqlTypeId::Text:
      if (!std::holds_alternative<std::string>(values[index])) {
        throw std::invalid_argument("Expected TEXT value");
      }
      appendText(out, std::get<std::string>(values[index]));
      break;
    default:
      throw std::invalid_argument(
          "Tuple codec supports only integer and TEXT columns");
    }
  }
  return out;
}

std::vector<unidb::sql::SqlValue>
SqlTupleCodec::decode(const std::vector<unidb::sql::ColumnDefinition> &schema,
                      const std::vector<char> &bytes) {
  constexpr std::uint8_t rowFormatVersion = 1;
  const bool versioned = usesDeclaredRowFormat(schema);
  std::size_t offset = 0;
  if (versioned &&
      readLittleEndian<std::uint8_t>(bytes, offset) != rowFormatVersion) {
    throw std::runtime_error("Unsupported SQL tuple row format version");
  }

  std::vector<unidb::sql::SqlValue> values;
  values.reserve(schema.size());
  for (const auto &column : schema) {
    if (!column.declaredType.has_value()) {
      if (column.type == unidb::sql::SqlType::Int) {
        values.emplace_back(readLittleEndian<std::int64_t>(bytes, offset));
      } else if (column.type == unidb::sql::SqlType::Text) {
        values.emplace_back(readText(bytes, offset));
      } else {
        throw std::invalid_argument("Unsupported SQL tuple column type");
      }
      continue;
    }

    using unidb::sql::SqlTypeId;
    switch (column.declaredType->id) {
    case SqlTypeId::SmallInt:
    case SqlTypeId::Integer:
    case SqlTypeId::BigInt:
      values.emplace_back(
          readDeclaredInteger(column.declaredType->id, bytes, offset));
      break;
    case SqlTypeId::Text:
      values.emplace_back(readText(bytes, offset));
      break;
    default:
      throw std::invalid_argument(
          "Tuple codec supports only integer and TEXT columns");
    }
  }
  if (offset != bytes.size()) {
    throw std::runtime_error("Tuple bytes contain trailing data");
  }
  return values;
}
