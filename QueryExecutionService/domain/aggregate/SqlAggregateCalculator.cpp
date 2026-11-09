#include "SqlAggregateCalculator.h"

#include "../predicate/WherePredicateEvaluator.h"
#include "../types/SqlNumericSemantics.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>

namespace QueryExecutionService::domain {
namespace {

using unidb::sql::AggregateFunction;
using unidb::sql::SqlValue;

std::string addMagnitude(const std::string &left, const std::string &right) {
  std::string result;
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

std::string subtractMagnitude(const std::string &larger,
                              const std::string &smaller) {
  std::string result;
  std::size_t leftIndex = larger.size();
  std::size_t rightIndex = smaller.size();
  int borrow = 0;
  while (leftIndex > 0) {
    int digit = larger[--leftIndex] - '0' - borrow;
    if (rightIndex > 0) {
      digit -= smaller[--rightIndex] - '0';
    }
    borrow = digit < 0 ? 1 : 0;
    if (borrow != 0) {
      digit += 10;
    }
    result.push_back(static_cast<char>('0' + digit));
  }
  while (result.size() > 1 && result.back() == '0') {
    result.pop_back();
  }
  std::reverse(result.begin(), result.end());
  return result;
}

unidb::sql::SqlNumericValue
sumNumericValues(const std::vector<SqlValue> &values) {
  using unidb::sql::SqlNumericValue;
  std::int32_t commonScale = std::numeric_limits<std::int32_t>::min();
  std::vector<SqlNumericValue> aligned;
  aligned.reserve(values.size());
  for (const auto &value : values) {
    const auto *numeric = std::get_if<SqlNumericValue>(&value);
    if (numeric == nullptr) {
      throw std::invalid_argument("SUM requires values of a consistent type");
    }
    (void)formatSqlNumericValue(*numeric);
    commonScale = std::max(commonScale, numeric->scale);
    aligned.push_back(*numeric);
  }

  bool totalNegative = false;
  std::string total = "0";
  for (auto &value : aligned) {
    const auto padding = static_cast<std::int64_t>(commonScale) - value.scale;
    if (padding > 100000) {
      throw std::overflow_error("NUMERIC SUM scale exceeds supported range");
    }
    value.coefficient.append(static_cast<std::size_t>(padding), '0');
    if (value.coefficient == "0") {
      continue;
    }
    if (total == "0") {
      total = value.coefficient;
      totalNegative = value.negative;
      continue;
    }
    if (totalNegative == value.negative) {
      total = addMagnitude(total, value.coefficient);
      continue;
    }

    const auto magnitudeOrder =
        total.size() == value.coefficient.size()
            ? total.compare(value.coefficient)
            : (total.size() < value.coefficient.size() ? -1 : 1);
    if (magnitudeOrder == 0) {
      total = "0";
      totalNegative = false;
    } else if (magnitudeOrder > 0) {
      total = subtractMagnitude(total, value.coefficient);
    } else {
      total = subtractMagnitude(value.coefficient, total);
      totalNegative = value.negative;
    }
  }
  return normalizeSqlNumericValue(totalNegative, std::move(total), commonScale);
}

std::string incrementMagnitude(std::string value) {
  for (std::size_t index = value.size(); index > 0; --index) {
    if (value[index - 1] != '9') {
      ++value[index - 1];
      return value;
    }
    value[index - 1] = '0';
  }
  value.insert(value.begin(), '1');
  return value;
}

unidb::sql::SqlNumericValue
averageNumericValues(const std::vector<SqlValue> &values) {
  using unidb::sql::SqlNumericValue;
  const auto sum = sumNumericValues(values);
  constexpr std::int32_t minimumResultScale = 16;
  const auto resultScale = std::max(sum.scale, minimumResultScale);
  const auto padding = static_cast<std::int64_t>(resultScale) - sum.scale;
  if (padding > 100000) {
    throw std::overflow_error("NUMERIC AVG scale exceeds supported range");
  }

  std::string numerator = sum.coefficient;
  numerator.append(static_cast<std::size_t>(padding), '0');
  const auto divisor = static_cast<std::uint64_t>(values.size());
  using WideUnsigned = unsigned __int128;
  WideUnsigned remainder = 0;
  std::string quotient;
  quotient.reserve(numerator.size());
  for (const char digit : numerator) {
    const auto current = remainder * 10 + static_cast<unsigned>(digit - '0');
    quotient.push_back(static_cast<char>('0' + current / divisor));
    remainder = current % divisor;
  }

  const auto firstSignificant = quotient.find_first_not_of('0');
  quotient = firstSignificant == std::string::npos
                 ? "0"
                 : quotient.substr(firstSignificant);
  if (remainder * 2 >= divisor) {
    quotient = incrementMagnitude(std::move(quotient));
  }
  return normalizeSqlNumericValue(sum.negative, std::move(quotient),
                                  resultScale);
}

std::string aggregateFunctionName(AggregateFunction function) {
  switch (function) {
  case AggregateFunction::Count:
    return "COUNT";
  case AggregateFunction::Min:
    return "MIN";
  case AggregateFunction::Max:
    return "MAX";
  case AggregateFunction::Sum:
    return "SUM";
  case AggregateFunction::Avg:
    return "AVG";
  }
  throw std::invalid_argument("Unknown aggregate function");
}

} // namespace

SqlValue calculateAggregate(AggregateFunction function,
                            const std::vector<SqlValue> &values,
                            std::size_t inputRowCount) {
  if (function == AggregateFunction::Count) {
    return static_cast<std::int64_t>(inputRowCount);
  }
  if (values.empty()) {
    throw std::invalid_argument(aggregateFunctionName(function) +
                                " cannot be evaluated on an empty result set");
  }

  if (function == AggregateFunction::Min ||
      function == AggregateFunction::Max) {
    SqlValue result = values.front();
    for (const auto &value : values) {
      const int comparison = compareSqlValues(value, result);
      if ((function == AggregateFunction::Min && comparison < 0) ||
          (function == AggregateFunction::Max && comparison > 0)) {
        result = value;
      }
    }
    return result;
  }

  if (std::holds_alternative<unidb::sql::SqlNumericValue>(values.front())) {
    if (function == AggregateFunction::Sum) {
      return sumNumericValues(values);
    }
    if (function == AggregateFunction::Avg) {
      return averageNumericValues(values);
    }
    throw std::invalid_argument("Unsupported NUMERIC aggregate");
  }

  if (std::holds_alternative<float>(values.front())) {
    if (function == AggregateFunction::Sum) {
      float sum = 0.0F;
      for (const auto &value : values) {
        sum += std::get<float>(value);
      }
      return sum;
    }

    double sum = 0.0;
    for (const auto &value : values) {
      sum += static_cast<double>(std::get<float>(value));
    }
    return sum / static_cast<double>(values.size());
  }

  if (std::holds_alternative<double>(values.front())) {
    double sum = 0.0;
    for (const auto &value : values) {
      sum += std::get<double>(value);
    }
    if (function == AggregateFunction::Avg) {
      sum /= static_cast<double>(values.size());
    }
    return sum;
  }

  std::int64_t sum = 0;
  for (const auto &value : values) {
    const auto integer = std::get<std::int64_t>(value);
    if ((integer > 0 &&
         sum > std::numeric_limits<std::int64_t>::max() - integer) ||
        (integer < 0 &&
         sum < std::numeric_limits<std::int64_t>::min() - integer)) {
      throw std::invalid_argument("SUM result exceeds INT64 range");
    }
    sum += integer;
  }
  if (function == AggregateFunction::Avg) {
    sum /= static_cast<std::int64_t>(values.size());
  }
  return sum;
}

} // namespace QueryExecutionService::domain
