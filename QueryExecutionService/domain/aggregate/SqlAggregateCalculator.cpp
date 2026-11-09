#include "SqlAggregateCalculator.h"

#include "../predicate/WherePredicateEvaluator.h"

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>

namespace QueryExecutionService::domain {
namespace {

using unidb::sql::AggregateFunction;
using unidb::sql::SqlValue;

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
