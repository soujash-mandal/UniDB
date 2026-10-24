#include "RdbRowCodec.h"

#include <limits>
#include <stdexcept>
#include <type_traits>

namespace {
template <typename T>
void appendLittleEndian(std::vector<char> &bytes, T value) {
  using Unsigned = std::make_unsigned_t<T>;
  const auto bits = static_cast<Unsigned>(value);
  for (std::size_t i = 0; i < sizeof(T); ++i) {
    bytes.push_back(static_cast<char>((bits >> (i * 8U)) & 0xFFU));
  }
}

template <typename T>
T readLittleEndian(const std::vector<char> &bytes, std::size_t &offset) {
  if (bytes.size() - offset < sizeof(T)) {
    throw std::runtime_error("Tuple data is truncated");
  }
  using Unsigned = std::make_unsigned_t<T>;
  Unsigned bits = 0;
  for (std::size_t i = 0; i < sizeof(T); ++i) {
    bits |= static_cast<Unsigned>(static_cast<unsigned char>(bytes[offset + i]))
            << (i * 8U);
  }
  offset += sizeof(T);
  return static_cast<T>(bits);
}
} // namespace

std::vector<char> RdbRowCodec::encode(const RdbTableDefinition &table,
                                      const RdbRow &row) {
  if (row.size() != table.columns.size()) {
    throw std::invalid_argument("Row value count does not match table schema");
  }

  std::vector<char> bytes;
  for (std::size_t i = 0; i < row.size(); ++i) {
    const auto &column = table.columns[i];
    if (column.type == RdbDataType::INT) {
      const auto *value = std::get_if<int64_t>(&row[i]);
      if (value == nullptr || *value < std::numeric_limits<int32_t>::min() ||
          *value > std::numeric_limits<int32_t>::max()) {
        throw std::invalid_argument("INT column requires a 32-bit integer");
      }
      appendLittleEndian<int32_t>(bytes, static_cast<int32_t>(*value));
    } else if (column.type == RdbDataType::BIGINT) {
      const auto *value = std::get_if<int64_t>(&row[i]);
      if (value == nullptr) {
        throw std::invalid_argument("BIGINT column requires an integer");
      }
      appendLittleEndian<int64_t>(bytes, *value);
    } else if (column.type == RdbDataType::VARCHAR) {
      const auto *value = std::get_if<std::string>(&row[i]);
      if (value == nullptr) {
        throw std::invalid_argument("VARCHAR column requires text");
      }
      if (column.size != 0 && value->size() > column.size) {
        throw std::length_error("Text value exceeds VARCHAR column size");
      }
      if (value->size() > std::numeric_limits<uint32_t>::max()) {
        throw std::length_error("Text value exceeds the V1 row size limit");
      }
      appendLittleEndian<uint32_t>(bytes, static_cast<uint32_t>(value->size()));
      bytes.insert(bytes.end(), value->begin(), value->end());
    } else {
      throw std::invalid_argument("Unsupported V1 row column type");
    }
  }
  return bytes;
}

RdbRow RdbRowCodec::decode(const RdbTableDefinition &table,
                           const std::vector<char> &bytes) {
  RdbRow row;
  row.reserve(table.columns.size());
  std::size_t offset = 0;
  for (const auto &column : table.columns) {
    if (column.type == RdbDataType::INT) {
      row.emplace_back(
          static_cast<int64_t>(readLittleEndian<int32_t>(bytes, offset)));
    } else if (column.type == RdbDataType::BIGINT) {
      row.emplace_back(readLittleEndian<int64_t>(bytes, offset));
    } else if (column.type == RdbDataType::VARCHAR) {
      const uint32_t length = readLittleEndian<uint32_t>(bytes, offset);
      if (length > bytes.size() - offset) {
        throw std::runtime_error("Tuple text value is truncated");
      }
      if (column.size != 0 && length > column.size) {
        throw std::runtime_error(
            "Tuple text value exceeds VARCHAR column size");
      }
      row.emplace_back(
          std::string(bytes.begin() + offset, bytes.begin() + offset + length));
      offset += length;
    } else {
      throw std::invalid_argument("Unsupported V1 row column type");
    }
  }
  if (offset != bytes.size()) {
    throw std::runtime_error("Tuple data contains trailing bytes");
  }
  return row;
}
