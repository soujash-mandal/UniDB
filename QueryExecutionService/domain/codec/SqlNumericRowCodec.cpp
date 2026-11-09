#include "SqlNumericRowCodec.h"

#include "../types/SqlNumericSemantics.h"

#include <bit>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace QueryExecutionService::domain {
namespace {

constexpr std::uint8_t rowFormatVersion = 1;

template <typename T>
void appendLittleEndian(std::vector<char> &bytes, T value) {
  static_assert(std::is_integral_v<T>);
  using Unsigned = std::make_unsigned_t<T>;
  const auto raw = static_cast<Unsigned>(value);
  for (std::size_t index = 0; index < sizeof(T); ++index) {
    bytes.push_back(static_cast<char>((raw >> (index * 8U)) & Unsigned{0xff}));
  }
}

template <typename T>
T readLittleEndian(const std::vector<char> &bytes, std::size_t &offset) {
  static_assert(std::is_integral_v<T>);
  if (offset > bytes.size() || sizeof(T) > bytes.size() - offset) {
    throw std::runtime_error("NUMERIC row value is truncated");
  }

  using Unsigned = std::make_unsigned_t<T>;
  Unsigned raw{};
  for (std::size_t index = 0; index < sizeof(T); ++index) {
    raw |=
        static_cast<Unsigned>(static_cast<unsigned char>(bytes[offset + index]))
        << (index * 8U);
  }
  offset += sizeof(T);
  if constexpr (std::is_signed_v<T>) {
    return std::bit_cast<T>(raw);
  }
  return raw;
}

} // namespace

std::vector<char>
SqlNumericRowCodec::encode(const unidb::sql::SqlNumericValue &value) {
  // Formatting validates that the value is already in canonical form.
  (void)formatSqlNumericValue(value);
  if (value.coefficient.size() > std::numeric_limits<std::uint32_t>::max()) {
    throw std::invalid_argument("NUMERIC coefficient exceeds row length limit");
  }

  std::vector<char> bytes;
  bytes.reserve(1 + 1 + sizeof(std::int32_t) + sizeof(std::uint32_t) +
                value.coefficient.size());
  appendLittleEndian(bytes, rowFormatVersion);
  appendLittleEndian(bytes, static_cast<std::uint8_t>(value.negative ? 1 : 0));
  appendLittleEndian(bytes, value.scale);
  appendLittleEndian(bytes,
                     static_cast<std::uint32_t>(value.coefficient.size()));
  bytes.insert(bytes.end(), value.coefficient.begin(), value.coefficient.end());
  return bytes;
}

unidb::sql::SqlNumericValue
SqlNumericRowCodec::decode(const std::vector<char> &bytes) {
  std::size_t offset = 0;
  if (readLittleEndian<std::uint8_t>(bytes, offset) != rowFormatVersion) {
    throw std::runtime_error("Unsupported NUMERIC row format version");
  }

  const auto sign = readLittleEndian<std::uint8_t>(bytes, offset);
  if (sign > 1) {
    throw std::runtime_error("NUMERIC row value has an invalid sign");
  }
  const auto scale = readLittleEndian<std::int32_t>(bytes, offset);
  const auto coefficientSize = readLittleEndian<std::uint32_t>(bytes, offset);
  if (coefficientSize == 0 || offset > bytes.size() ||
      coefficientSize != bytes.size() - offset) {
    throw std::runtime_error("NUMERIC row coefficient has an invalid length");
  }

  std::string coefficient(bytes.data() + offset, coefficientSize);
  const auto value =
      normalizeSqlNumericValue(sign == 1, std::move(coefficient), scale);
  if (value.negative != (sign == 1) ||
      value.coefficient !=
          std::string(bytes.data() + offset, coefficientSize) ||
      value.scale != scale) {
    throw std::runtime_error("NUMERIC row value is not canonical");
  }
  return value;
}

} // namespace QueryExecutionService::domain
