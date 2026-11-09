#include "WherePredicateEvaluator.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>

namespace QueryExecutionService::domain {
namespace {
using unidb::sql::ColumnDefinition;
using unidb::sql::ComparisonOperator;
using unidb::sql::SqlType;
using unidb::sql::SqlValue;
using unidb::sql::WhereEquals;
using unidb::sql::WhereExpression;

template <typename T> int compareFloatingPoint(T left, T right) {
  const bool leftIsNaN = std::isnan(left);
  const bool rightIsNaN = std::isnan(right);
  if (leftIsNaN || rightIsNaN) {
    return leftIsNaN == rightIsNaN ? 0 : (leftIsNaN ? 1 : -1);
  }
  return (left > right) - (left < right);
}
} // namespace

int compareSqlValues(const SqlValue &left, const SqlValue &right) {
  if (left.index() != right.index()) {
    throw std::invalid_argument(
        "WHERE comparison value type does not match column type");
  }
  if (const auto *leftInt = std::get_if<std::int64_t>(&left)) {
    const auto rightInt = std::get<std::int64_t>(right);
    return (*leftInt > rightInt) - (*leftInt < rightInt);
  }
  if (const auto *leftReal = std::get_if<float>(&left)) {
    return compareFloatingPoint(*leftReal, std::get<float>(right));
  }
  if (const auto *leftDouble = std::get_if<double>(&left)) {
    return compareFloatingPoint(*leftDouble, std::get<double>(right));
  }
  if (const auto *leftBoolean = std::get_if<bool>(&left)) {
    const bool rightBoolean = std::get<bool>(right);
    return (*leftBoolean > rightBoolean) - (*leftBoolean < rightBoolean);
  }
  const auto &leftText = std::get<std::string>(left);
  const auto &rightText = std::get<std::string>(right);
  return (leftText > rightText) - (leftText < rightText);
}

bool valueMatchesColumn(const ColumnDefinition &column, const SqlValue &value) {
  using unidb::sql::SqlTypeId;
  if (column.declaredType.has_value()) {
    switch (column.declaredType->id) {
    case SqlTypeId::Boolean:
      return std::holds_alternative<bool>(value);
    case SqlTypeId::SmallInt:
    case SqlTypeId::Integer:
    case SqlTypeId::BigInt:
      return std::holds_alternative<std::int64_t>(value);
    case SqlTypeId::Real:
      return std::holds_alternative<float>(value);
    case SqlTypeId::DoublePrecision:
      return std::holds_alternative<double>(value);
    case SqlTypeId::Text:
      return std::holds_alternative<std::string>(value);
    default:
      return false;
    }
  }
  return column.type == SqlType::Int
             ? std::holds_alternative<std::int64_t>(value)
             : std::holds_alternative<std::string>(value);
}

bool matchesLikePattern(const std::string &value, const std::string &pattern) {
  std::size_t valueIndex = 0;
  std::size_t patternIndex = 0;
  std::size_t lastPercent = std::string::npos;
  std::size_t retryValueIndex = 0;

  while (valueIndex < value.size()) {
    if (patternIndex < pattern.size() &&
        (pattern[patternIndex] == '_' ||
         pattern[patternIndex] == value[valueIndex])) {
      ++valueIndex;
      ++patternIndex;
    } else if (patternIndex < pattern.size() && pattern[patternIndex] == '%') {
      lastPercent = patternIndex++;
      retryValueIndex = valueIndex;
    } else if (lastPercent != std::string::npos) {
      patternIndex = lastPercent + 1;
      valueIndex = ++retryValueIndex;
    } else {
      return false;
    }
  }

  while (patternIndex < pattern.size() && pattern[patternIndex] == '%') {
    ++patternIndex;
  }
  return patternIndex == pattern.size();
}

bool matchesWhere(const SqlValue &rowValue, const WhereEquals &predicate) {
  if (predicate.comparison == ComparisonOperator::Like ||
      predicate.comparison == ComparisonOperator::NotLike) {
    const auto *textValue = std::get_if<std::string>(&rowValue);
    const auto *pattern = std::get_if<std::string>(&predicate.value);
    if (textValue == nullptr || pattern == nullptr) {
      throw std::invalid_argument("LIKE predicates require TEXT values");
    }
    const bool matched = matchesLikePattern(*textValue, *pattern);
    return predicate.comparison == ComparisonOperator::Like ? matched
                                                            : !matched;
  }
  if (predicate.comparison == ComparisonOperator::Between ||
      predicate.comparison == ComparisonOperator::NotBetween) {
    if (!predicate.upperBound.has_value()) {
      throw std::invalid_argument(
          "BETWEEN predicates require two range bounds");
    }
    const bool withinRange =
        compareSqlValues(rowValue, predicate.value) >= 0 &&
        compareSqlValues(rowValue, *predicate.upperBound) <= 0;
    return predicate.comparison == ComparisonOperator::Between ? withinRange
                                                               : !withinRange;
  }
  if (predicate.comparison == ComparisonOperator::In ||
      predicate.comparison == ComparisonOperator::NotIn) {
    const bool found =
        std::any_of(predicate.values.begin(), predicate.values.end(),
                    [&](const auto &value) {
                      return compareSqlValues(rowValue, value) == 0;
                    });
    return predicate.comparison == ComparisonOperator::In ? found : !found;
  }

  const int comparison = compareSqlValues(rowValue, predicate.value);
  switch (predicate.comparison) {
  case ComparisonOperator::Equal:
    return comparison == 0;
  case ComparisonOperator::NotEqual:
    return comparison != 0;
  case ComparisonOperator::GreaterThan:
    return comparison > 0;
  case ComparisonOperator::LessThan:
    return comparison < 0;
  case ComparisonOperator::GreaterThanOrEqual:
    return comparison >= 0;
  case ComparisonOperator::LessThanOrEqual:
    return comparison <= 0;
  case ComparisonOperator::In:
  case ComparisonOperator::NotIn:
    throw std::invalid_argument("IN predicates require a value list");
  case ComparisonOperator::Between:
  case ComparisonOperator::NotBetween:
    throw std::invalid_argument("BETWEEN predicates require two range bounds");
  case ComparisonOperator::Like:
  case ComparisonOperator::NotLike:
    throw std::invalid_argument("LIKE predicates require a text pattern");
  }
  throw std::invalid_argument("Unknown WHERE comparison operator");
}

CompiledWhereExpression
compileWhereExpression(const WhereExpression &expression,
                       const std::vector<ColumnDefinition> &schema,
                       const std::vector<SqlType> &types) {
  CompiledWhereExpression compiled;
  compiled.kind = expression.kind;
  if (expression.kind == WhereExpression::Kind::Predicate) {
    const auto found =
        std::find_if(schema.begin(), schema.end(), [&](const auto &column) {
          return column.name == expression.predicate.columnName;
        });
    if (found == schema.end()) {
      throw std::invalid_argument("Unknown WHERE column: " +
                                  expression.predicate.columnName);
    }
    compiled.columnIndex =
        static_cast<std::size_t>(std::distance(schema.begin(), found));
    const bool isMembership =
        expression.predicate.comparison == ComparisonOperator::In ||
        expression.predicate.comparison == ComparisonOperator::NotIn;
    const bool isRange =
        expression.predicate.comparison == ComparisonOperator::Between ||
        expression.predicate.comparison == ComparisonOperator::NotBetween;
    const bool isLike =
        expression.predicate.comparison == ComparisonOperator::Like ||
        expression.predicate.comparison == ComparisonOperator::NotLike;
    if (isLike && types[compiled.columnIndex] != SqlType::Text) {
      throw std::invalid_argument("LIKE requires a TEXT column: " +
                                  found->name);
    }
    if (!isMembership &&
        !valueMatchesColumn(*found, expression.predicate.value)) {
      throw std::invalid_argument("WHERE value type does not match column: " +
                                  found->name);
    }
    if (isRange) {
      if (!expression.predicate.upperBound.has_value()) {
        throw std::invalid_argument(
            "BETWEEN predicates require two range bounds");
      }
      if (!valueMatchesColumn(*found, *expression.predicate.upperBound)) {
        throw std::invalid_argument("WHERE value type does not match column: " +
                                    found->name);
      }
    }
    if (isLike &&
        !std::holds_alternative<std::string>(expression.predicate.value)) {
      throw std::invalid_argument("LIKE pattern must be TEXT");
    }
    if (isMembership) {
      if (expression.predicate.values.empty()) {
        throw std::invalid_argument("IN list must contain at least one value");
      }
      for (const auto &value : expression.predicate.values) {
        if (!valueMatchesColumn(*found, value)) {
          throw std::invalid_argument(
              "WHERE value type does not match column: " + found->name);
        }
      }
    }
    compiled.predicate = expression.predicate;
    return compiled;
  }

  if (expression.children.empty()) {
    throw std::invalid_argument("WHERE logical expression cannot be empty");
  }
  compiled.children.reserve(expression.children.size());
  for (const auto &child : expression.children) {
    compiled.children.push_back(compileWhereExpression(child, schema, types));
  }
  return compiled;
}

bool matchesWhereExpression(const std::vector<SqlValue> &row,
                            const CompiledWhereExpression &expression) {
  if (expression.kind == WhereExpression::Kind::Predicate) {
    return matchesWhere(row[expression.columnIndex], expression.predicate);
  }
  if (expression.kind == WhereExpression::Kind::And) {
    return std::all_of(
        expression.children.begin(), expression.children.end(),
        [&](const auto &child) { return matchesWhereExpression(row, child); });
  }
  return std::any_of(
      expression.children.begin(), expression.children.end(),
      [&](const auto &child) { return matchesWhereExpression(row, child); });
}

} // namespace QueryExecutionService::domain
