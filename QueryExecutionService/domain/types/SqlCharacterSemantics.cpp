#include "SqlCharacterSemantics.h"

#include <stdexcept>

namespace QueryExecutionService::domain {
namespace {

bool isContinuation(unsigned char value) {
  return (value & 0xc0U) == 0x80U;
}

std::size_t utf8SequenceLength(std::string_view value, std::size_t offset) {
  const auto first = static_cast<unsigned char>(value[offset]);
  if (first <= 0x7fU) {
    return 1;
  }
  if (first >= 0xc2U && first <= 0xdfU) {
    if (offset + 1 >= value.size() ||
        !isContinuation(static_cast<unsigned char>(value[offset + 1]))) {
      throw std::invalid_argument("Character value contains invalid UTF-8");
    }
    return 2;
  }
  if (first >= 0xe0U && first <= 0xefU) {
    if (offset + 2 >= value.size()) {
      throw std::invalid_argument("Character value contains invalid UTF-8");
    }
    const auto second = static_cast<unsigned char>(value[offset + 1]);
    const auto third = static_cast<unsigned char>(value[offset + 2]);
    const bool validSecond =
        (first == 0xe0U && second >= 0xa0U && second <= 0xbfU) ||
        (first == 0xedU && second >= 0x80U && second <= 0x9fU) ||
        ((first != 0xe0U && first != 0xedU) && isContinuation(second));
    if (!validSecond || !isContinuation(third)) {
      throw std::invalid_argument("Character value contains invalid UTF-8");
    }
    return 3;
  }
  if (first >= 0xf0U && first <= 0xf4U) {
    if (offset + 3 >= value.size()) {
      throw std::invalid_argument("Character value contains invalid UTF-8");
    }
    const auto second = static_cast<unsigned char>(value[offset + 1]);
    const auto third = static_cast<unsigned char>(value[offset + 2]);
    const auto fourth = static_cast<unsigned char>(value[offset + 3]);
    const bool validSecond =
        (first == 0xf0U && second >= 0x90U && second <= 0xbfU) ||
        (first == 0xf4U && second >= 0x80U && second <= 0x8fU) ||
        ((first != 0xf0U && first != 0xf4U) && isContinuation(second));
    if (!validSecond || !isContinuation(third) || !isContinuation(fourth)) {
      throw std::invalid_argument("Character value contains invalid UTF-8");
    }
    return 4;
  }
  throw std::invalid_argument("Character value contains invalid UTF-8");
}

std::size_t byteOffsetForCharacters(std::string_view value,
                                    std::size_t characterCount) {
  std::size_t offset = 0;
  for (std::size_t index = 0; index < characterCount; ++index) {
    if (offset == value.size()) {
      return offset;
    }
    offset += utf8SequenceLength(value, offset);
  }
  return offset;
}

bool onlySpaces(std::string_view value) {
  for (const char character : value) {
    if (character != ' ') {
      return false;
    }
  }
  return true;
}

} // namespace

std::size_t countSqlCharacters(std::string_view value) {
  std::size_t count = 0;
  for (std::size_t offset = 0; offset < value.size(); ++count) {
    offset += utf8SequenceLength(value, offset);
  }
  return count;
}

std::string normalizeSqlCharacterValue(
    const unidb::sql::SqlTypeDescriptor &type, std::string_view value) {
  using unidb::sql::SqlTypeId;

  const auto characterCount = countSqlCharacters(value);
  if (type.id == SqlTypeId::Text) {
    return std::string(value);
  }
  if (type.id != SqlTypeId::Char && type.id != SqlTypeId::Varchar) {
    throw std::invalid_argument("SQL type is not a character type");
  }

  const auto maximumLength =
      type.id == SqlTypeId::Char
          ? type.modifier.length.value_or(1U)
          : type.modifier.length.value_or(0U);
  if (maximumLength == 0U) {
    return std::string(value);
  }

  std::string normalized(value);
  if (characterCount > maximumLength) {
    const auto boundary = byteOffsetForCharacters(value, maximumLength);
    const auto excess = value.substr(boundary);
    if (!onlySpaces(excess)) {
      throw std::invalid_argument("Character value exceeds declared length");
    }
    normalized.resize(boundary);
  }

  if (type.id == SqlTypeId::Char) {
    const auto storedLength = countSqlCharacters(normalized);
    normalized.append(maximumLength - storedLength, ' ');
  }
  return normalized;
}

} // namespace QueryExecutionService::domain
