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
      total.assign(1, '0');
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

unidb::sql::SqlNumericValue
averageNumericValues(const std::vector<SqlValue> &values) {
  const auto sum = sumNumericValues(values);
  const auto count = parseSqlNumericLiteral(std::to_string(values.size()));
  const auto resultScale = selectSqlNumericDivisionScale(sum, count);
  return divideSqlNumericValues(sum, count, resultScale);
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
                            std::size_t inputRowCount, bool countAllRows) {
  if (function == AggregateFunction::Count) {
    if (countAllRows) {
      return static_cast<std::int64_t>(inputRowCount);
    }
    return static_cast<std::int64_t>(std::count_if(
        values.begin(), values.end(), [](const auto &value) {
          return !std::holds_alternative<unidb::sql::SqlNullValue>(value);
        }));
  }

  std::vector<SqlValue> nonNullValues;
  nonNullValues.reserve(values.size());
  for (const auto &value : values) {
    if (!std::holds_alternative<unidb::sql::SqlNullValue>(value)) {
      nonNullValues.push_back(value);
    }
  }
  if (nonNullValues.empty()) {
    return unidb::sql::SqlNullValue{};
  }
  const auto &aggregateInputs = nonNullValues;

  if (function == AggregateFunction::Min ||
      function == AggregateFunction::Max) {
    SqlValue result = aggregateInputs.front();
    for (const auto &value : aggregateInputs) {
      const int comparison = compareSqlValues(value, result);
      if ((function == AggregateFunction::Min && comparison < 0) ||
          (function == AggregateFunction::Max && comparison > 0)) {
        result = value;
      }
    }
    return result;
  }

  if (std::holds_alternative<unidb::sql::SqlNumericValue>(aggregateInputs.front())) {
    if (function == AggregateFunction::Sum) {
      return sumNumericValues(aggregateInputs);
    }
    if (function == AggregateFunction::Avg) {
      return averageNumericValues(aggregateInputs);
    }
    throw std::invalid_argument("Unsupported NUMERIC aggregate");
  }

  if (std::holds_alternative<float>(aggregateInputs.front())) {
    if (function == AggregateFunction::Sum) {
      float sum = 0.0F;
      for (const auto &value : aggregateInputs) {
        sum += std::get<float>(value);
      }
      return sum;
    }

    double sum = 0.0;
    for (const auto &value : aggregateInputs) {
      sum += static_cast<double>(std::get<float>(value));
    }
    return sum / static_cast<double>(aggregateInputs.size());
  }

  if (std::holds_alternative<double>(aggregateInputs.front())) {
    double sum = 0.0;
    for (const auto &value : aggregateInputs) {
      sum += std::get<double>(value);
    }
    if (function == AggregateFunction::Avg) {
      sum /= static_cast<double>(aggregateInputs.size());
    }
    return sum;
  }

  std::int64_t sum = 0;
  for (const auto &value : aggregateInputs) {
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
    sum /= static_cast<std::int64_t>(aggregateInputs.size());
  }
  return sum;
}

} // namespace QueryExecutionService::domain
