#include "SqlNumericSemantics.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace QueryExecutionService::domain {
namespace {

void validateCanonicalValue(const unidb::sql::SqlNumericValue &value) {
  if (value.coefficient.empty() ||
      !std::all_of(value.coefficient.begin(), value.coefficient.end(),
                   [](const unsigned char character) {
                     return character >= '0' && character <= '9';
                   })) {
    throw std::invalid_argument("NUMERIC coefficient must contain digits");
  }

  if (value.coefficient.size() > 1 &&
      (value.coefficient.front() == '0' || value.coefficient.back() == '0')) {
    throw std::invalid_argument("NUMERIC coefficient is not normalized");
  }
  if (value.coefficient == "0" && (value.negative || value.scale != 0)) {
    throw std::invalid_argument("NUMERIC zero is not normalized");
  }
}

int compareMagnitude(const unidb::sql::SqlNumericValue &left,
                     const unidb::sql::SqlNumericValue &right) {
  const auto leftIntegerDigits =
      static_cast<std::int64_t>(left.coefficient.size()) - left.scale;
  const auto rightIntegerDigits =
      static_cast<std::int64_t>(right.coefficient.size()) - right.scale;
  if (leftIntegerDigits != rightIntegerDigits) {
    return leftIntegerDigits < rightIntegerDigits ? -1 : 1;
  }

  const auto commonLength =
      std::max(left.coefficient.size(), right.coefficient.size());
  for (std::size_t index = 0; index < commonLength; ++index) {
    const char leftDigit =
        index < left.coefficient.size() ? left.coefficient[index] : '0';
    const char rightDigit =
        index < right.coefficient.size() ? right.coefficient[index] : '0';
    if (leftDigit != rightDigit) {
      return leftDigit < rightDigit ? -1 : 1;
    }
  }
  return 0;
}

int compareUnsignedMagnitudes(const std::string &left,
                              const std::string &right) {
  if (left.size() != right.size()) {
    return left.size() < right.size() ? -1 : 1;
  }
  const int comparison = left.compare(right);
  return comparison < 0 ? -1 : (comparison > 0 ? 1 : 0);
}

std::string addMagnitudes(const std::string &left, const std::string &right) {
  std::string result;
  result.reserve(std::max(left.size(), right.size()) + 1);
  std::size_t leftIndex = left.size();
  std::size_t rightIndex = right.size();
  int carry = 0;
  while (leftIndex > 0 || rightIndex > 0 || carry != 0) {
    int digit = carry;
    if (leftIndex > 0) {
      digit += left[--leftIndex] - '0';
    }
    if (rightIndex > 0) {
      digit += right[--rightIndex] - '0';
    }
    result.push_back(static_cast<char>('0' + digit % 10));
    carry = digit / 10;
  }
  std::reverse(result.begin(), result.end());
  return result;
}

std::string subtractMagnitudes(const std::string &larger,
                               const std::string &smaller) {
  std::string result;
  result.reserve(larger.size());
  std::size_t largerIndex = larger.size();
  std::size_t smallerIndex = smaller.size();
  int borrow = 0;
  while (largerIndex > 0) {
    int digit = larger[--largerIndex] - '0' - borrow;
    if (smallerIndex > 0) {
      digit -= smaller[--smallerIndex] - '0';
    }
    borrow = digit < 0 ? 1 : 0;
    if (borrow != 0) {
      digit += 10;
    }
    result.push_back(static_cast<char>('0' + digit));
  }
  std::reverse(result.begin(), result.end());
  const auto firstNonZero = result.find_first_not_of('0');
  return firstNonZero == std::string::npos ? "0" : result.substr(firstNonZero);
}

std::string multiplyMagnitudes(const std::string &left,
                               const std::string &right) {
  if (left == "0" || right == "0") {
    return "0";
  }
  if (left.size() > std::string{}.max_size() - right.size()) {
    throw std::length_error("NUMERIC multiplication result is too large");
  }
  std::vector<int> digits(left.size() + right.size(), 0);
  for (std::size_t i = left.size(); i > 0; --i) {
    int carry = 0;
    for (std::size_t j = right.size(); j > 0; --j) {
      const auto position = i + j - 1;
      const int product =
          (left[i - 1] - '0') * (right[j - 1] - '0') + digits[position] + carry;
      digits[position] = product % 10;
      carry = product / 10;
    }
    digits[i - 1] += carry;
  }
  std::string result;
  result.reserve(digits.size());
  const std::size_t first = digits.front() == 0 ? 1 : 0;
  for (std::size_t i = first; i < digits.size(); ++i) {
    result.push_back(static_cast<char>('0' + digits[i]));
  }
  return result;
}

std::pair<std::string, std::string>
alignNumericCoefficients(const unidb::sql::SqlNumericValue &left,
                         const unidb::sql::SqlNumericValue &right,
                         std::int32_t &resultScale) {
  resultScale = std::max(left.scale, right.scale);
  std::string leftCoefficient = left.coefficient;
  std::string rightCoefficient = right.coefficient;
  const auto leftZeros = static_cast<std::uint64_t>(
      static_cast<std::int64_t>(resultScale) - left.scale);
  const auto rightZeros = static_cast<std::uint64_t>(
      static_cast<std::int64_t>(resultScale) - right.scale);
  if (leftZeros > leftCoefficient.max_size() - leftCoefficient.size() ||
      rightZeros > rightCoefficient.max_size() - rightCoefficient.size()) {
    throw std::length_error("NUMERIC arithmetic result is too large");
  }
  leftCoefficient.append(static_cast<std::size_t>(leftZeros), '0');
  rightCoefficient.append(static_cast<std::size_t>(rightZeros), '0');
  return {std::move(leftCoefficient), std::move(rightCoefficient)};
}

} // namespace

unidb::sql::SqlNumericValue parseSqlNumericLiteral(std::string_view literal) {
  if (literal.empty()) {
    throw std::invalid_argument("NUMERIC literal cannot be empty");
  }

  std::size_t position = 0;
  bool negative = false;
  if (literal[position] == '+' || literal[position] == '-') {
    negative = literal[position] == '-';
    ++position;
  }
  if (position == literal.size()) {
    throw std::invalid_argument("NUMERIC literal requires digits");
  }

  std::string coefficient;
  coefficient.reserve(literal.size());
  std::int64_t fractionalDigits = 0;
  bool decimalPointSeen = false;
  bool digitSeen = false;
  while (position < literal.size()) {
    const char character = literal[position];
    if (character >= '0' && character <= '9') {
      coefficient.push_back(character);
      digitSeen = true;
      if (decimalPointSeen) {
        ++fractionalDigits;
      }
      ++position;
      continue;
    }
    if (character == '.' && !decimalPointSeen) {
      decimalPointSeen = true;
      ++position;
      continue;
    }
    break;
  }
  if (!digitSeen) {
    throw std::invalid_argument("NUMERIC literal requires digits");
  }

  std::int64_t exponent = 0;
  if (position < literal.size() &&
      (literal[position] == 'e' || literal[position] == 'E')) {
    ++position;
    bool exponentNegative = false;
    if (position < literal.size() &&
        (literal[position] == '+' || literal[position] == '-')) {
      exponentNegative = literal[position] == '-';
      ++position;
    }
    const auto exponentStart = position;
    while (position < literal.size() && literal[position] >= '0' &&
           literal[position] <= '9') {
      ++position;
    }
    if (position == exponentStart) {
      throw std::invalid_argument("NUMERIC exponent requires digits");
    }
    std::uint64_t magnitude = 0;
    const auto parsed = std::from_chars(literal.data() + exponentStart,
                                        literal.data() + position, magnitude);
    if (parsed.ec != std::errc{} || parsed.ptr != literal.data() + position) {
      throw std::out_of_range("NUMERIC exponent is out of range");
    }
    const auto maximumMagnitude =
        static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max()) +
        static_cast<std::uint64_t>(literal.size()) + 1U;
    if (magnitude > maximumMagnitude) {
      throw std::out_of_range("NUMERIC exponent is out of range");
    }
    exponent = static_cast<std::int64_t>(magnitude);
    if (exponentNegative) {
      exponent = -exponent;
    }
  }
  if (position != literal.size()) {
    throw std::invalid_argument("Invalid character in NUMERIC literal");
  }

  const auto scale = fractionalDigits - exponent;
  if (scale < std::numeric_limits<std::int32_t>::min() ||
      scale > std::numeric_limits<std::int32_t>::max()) {
    throw std::out_of_range("NUMERIC scale is out of range");
  }

  return normalizeSqlNumericValue(negative, std::move(coefficient),
                                  static_cast<std::int32_t>(scale));
}

unidb::sql::SqlNumericValue normalizeSqlNumericValue(bool negative,
                                                     std::string coefficient,
                                                     std::int32_t scale) {
  if (coefficient.empty() ||
      !std::all_of(coefficient.begin(), coefficient.end(),
                   [](const unsigned char character) {
                     return character >= '0' && character <= '9';
                   })) {
    throw std::invalid_argument("NUMERIC coefficient must contain digits");
  }

  const auto firstNonZero = coefficient.find_first_not_of('0');
  if (firstNonZero == std::string::npos) {
    return {false, "0", 0};
  }
  coefficient.erase(0, firstNonZero);

  while (coefficient.size() > 1 && coefficient.back() == '0') {
    if (scale == std::numeric_limits<std::int32_t>::min()) {
      throw std::out_of_range("NUMERIC scale is out of range");
    }
    coefficient.pop_back();
    --scale;
  }

  return {negative, std::move(coefficient), scale};
}

std::string formatSqlNumericValue(const unidb::sql::SqlNumericValue &value) {
  validateCanonicalValue(value);

  std::string formatted;
  if (value.scale <= 0) {
    const auto trailingZeroCount =
        static_cast<std::uint64_t>(-static_cast<std::int64_t>(value.scale));
    if (trailingZeroCount > formatted.max_size() - value.coefficient.size()) {
      throw std::length_error("NUMERIC value is too large to format");
    }
    formatted = value.coefficient;
    formatted.append(static_cast<std::size_t>(trailingZeroCount), '0');
  } else {
    const auto scale = static_cast<std::size_t>(value.scale);
    if (scale >= value.coefficient.size()) {
      formatted = "0.";
      formatted.append(scale - value.coefficient.size(), '0');
      formatted += value.coefficient;
    } else {
      const auto integerLength = value.coefficient.size() - scale;
      formatted = value.coefficient.substr(0, integerLength);
      formatted.push_back('.');
      formatted += value.coefficient.substr(integerLength);
    }
  }

  if (value.negative) {
    formatted.insert(formatted.begin(), '-');
  }
  return formatted;
}

int compareSqlNumericValues(const unidb::sql::SqlNumericValue &left,
                            const unidb::sql::SqlNumericValue &right) {
  validateCanonicalValue(left);
  validateCanonicalValue(right);

  if (left.coefficient == "0" && right.coefficient == "0") {
    return 0;
  }
  if (left.negative != right.negative) {
    return left.negative ? -1 : 1;
  }

  const int magnitude = compareMagnitude(left, right);
  return left.negative ? -magnitude : magnitude;
}

unidb::sql::SqlNumericValue
addSqlNumericValues(const unidb::sql::SqlNumericValue &left,
                    const unidb::sql::SqlNumericValue &right) {
  (void)compareSqlNumericValues(left, left);
  (void)compareSqlNumericValues(right, right);
  std::int32_t scale = 0;
  auto [leftCoefficient, rightCoefficient] =
      alignNumericCoefficients(left, right, scale);
  if (left.negative == right.negative) {
    return normalizeSqlNumericValue(
        left.negative, addMagnitudes(leftCoefficient, rightCoefficient), scale);
  }
  const int comparison =
      compareUnsignedMagnitudes(leftCoefficient, rightCoefficient);
  if (comparison == 0) {
    return normalizeSqlNumericValue(false, "0", 0);
  }
  if (comparison > 0) {
    return normalizeSqlNumericValue(
        left.negative, subtractMagnitudes(leftCoefficient, rightCoefficient),
        scale);
  }
  return normalizeSqlNumericValue(
      right.negative, subtractMagnitudes(rightCoefficient, leftCoefficient),
      scale);
}

unidb::sql::SqlNumericValue
subtractSqlNumericValues(const unidb::sql::SqlNumericValue &left,
                         const unidb::sql::SqlNumericValue &right) {
  (void)compareSqlNumericValues(left, left);
  (void)compareSqlNumericValues(right, right);
  auto negatedRight = right;
  if (negatedRight.coefficient != "0") {
    negatedRight.negative = !negatedRight.negative;
  }
  return addSqlNumericValues(left, negatedRight);
}

unidb::sql::SqlNumericValue
multiplySqlNumericValues(const unidb::sql::SqlNumericValue &left,
                         const unidb::sql::SqlNumericValue &right) {
  (void)compareSqlNumericValues(left, left);
  (void)compareSqlNumericValues(right, right);
  const auto scale = static_cast<std::int64_t>(left.scale) + right.scale;
  if (scale < std::numeric_limits<std::int32_t>::min() ||
      scale > std::numeric_limits<std::int32_t>::max()) {
    throw std::out_of_range("NUMERIC multiplication scale is out of range");
  }
  if (left.coefficient == "0" || right.coefficient == "0") {
    return normalizeSqlNumericValue(false, "0", 0);
  }
  return normalizeSqlNumericValue(
      left.negative != right.negative,
      multiplyMagnitudes(left.coefficient, right.coefficient),
      static_cast<std::int32_t>(scale));
}

} // namespace QueryExecutionService::domain
