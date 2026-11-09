#include "SqlNumericSemantics.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <limits>
#include <stdexcept>
#include <utility>

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
applySqlNumericTypmod(const unidb::sql::SqlNumericValue &value,
                      std::uint32_t precision, std::int32_t scale) {
  validateCanonicalValue(value);
  if (precision == 0 || precision > 1000) {
    throw std::invalid_argument("NUMERIC precision must be between 1 and 1000");
  }
  if (scale < -1000 || scale > 1000) {
    throw std::invalid_argument("NUMERIC scale must be between -1000 and 1000");
  }
  if (value.coefficient == "0") {
    return value;
  }

  std::string roundedCoefficient = value.coefficient;
  const auto shift = static_cast<std::int64_t>(scale) - value.scale;
  if (shift > 0) {
    if (static_cast<std::uint64_t>(shift) >
        roundedCoefficient.max_size() - roundedCoefficient.size()) {
      throw std::out_of_range("NUMERIC value exceeds declared precision");
    }
    roundedCoefficient.append(static_cast<std::size_t>(shift), '0');
  } else if (shift < 0) {
    // PostgreSQL NUMERIC rounds halfway cases away from zero; the sign is
    // applied after rounding the absolute coefficient.
    const auto discardedDigits = static_cast<std::uint64_t>(-shift);
    bool roundUp = false;
    if (discardedDigits <= roundedCoefficient.size()) {
      const auto firstDiscarded =
          roundedCoefficient[roundedCoefficient.size() -
                             static_cast<std::size_t>(discardedDigits)];
      roundUp = firstDiscarded >= '5';
    }

    if (discardedDigits >= roundedCoefficient.size()) {
      roundedCoefficient = roundUp ? "1" : "0";
    } else {
      roundedCoefficient.resize(roundedCoefficient.size() -
                                static_cast<std::size_t>(discardedDigits));
      if (roundUp) {
        std::size_t position = roundedCoefficient.size();
        while (position > 0 && roundedCoefficient[position - 1] == '9') {
          roundedCoefficient[position - 1] = '0';
          --position;
        }
        if (position == 0) {
          roundedCoefficient.insert(roundedCoefficient.begin(), '1');
        } else {
          ++roundedCoefficient[position - 1];
        }
      }
    }
  }

  auto rounded = normalizeSqlNumericValue(value.negative,
                                          std::move(roundedCoefficient), scale);
  if (rounded.coefficient == "0") {
    return rounded;
  }

  const auto integerDigits =
      static_cast<std::int64_t>(rounded.coefficient.size()) - rounded.scale;
  const auto maximumIntegerDigits =
      static_cast<std::int64_t>(precision) - scale;
  if (integerDigits > maximumIntegerDigits) {
    throw std::out_of_range("NUMERIC value overflows declared precision");
  }
  return rounded;
}

} // namespace QueryExecutionService::domain
