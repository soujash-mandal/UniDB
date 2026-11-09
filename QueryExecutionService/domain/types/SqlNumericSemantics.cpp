#include "SqlNumericSemantics.h"

#include <algorithm>
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

} // namespace QueryExecutionService::domain
