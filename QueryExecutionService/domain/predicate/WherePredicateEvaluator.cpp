#include "WherePredicateEvaluator.h"

#include "../types/SqlCharacterSemantics.h"
#include "../types/SqlNumericSemantics.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace QueryExecutionService::domain {
namespace {
using unidb::sql::ColumnDefinition;
using unidb::sql::ComparisonOperator;
using unidb::sql::SqlType;
using unidb::sql::SqlValue;
using unidb::sql::WhereEquals;
using unidb::sql::WhereExpression;

void normalizeComparisonValue(const ColumnDefinition &column, SqlValue &value) {
  using unidb::sql::SqlTypeId;
  if (std::holds_alternative<unidb::sql::SqlNullValue>(value)) {
    return;
  }
  if (!column.declaredType.has_value()) {
    return;
  }

  const auto type = column.declaredType->id;
  if (type == SqlTypeId::Numeric) {
    if (const auto *integer = std::get_if<std::int64_t>(&value)) {
      value = parseSqlNumericLiteral(std::to_string(*integer));
    }
    return;
  }

  if (type != SqlTypeId::Real && type != SqlTypeId::DoublePrecision) {
    return;
  }

  if (const auto *numeric = std::get_if<unidb::sql::SqlNumericValue>(&value)) {
    const auto text = formatSqlNumericValue(*numeric);
    if (type == SqlTypeId::Real) {
      float converted{};
      const auto result =
          std::from_chars(text.data(), text.data() + text.size(), converted,
                          std::chars_format::general);
      if (result.ec != std::errc{} || result.ptr != text.data() + text.size() ||
          !std::isfinite(converted)) {
        throw std::invalid_argument("REAL comparison value is out of range");
      }
      value = converted;
    } else {
      double converted{};
      const auto result =
          std::from_chars(text.data(), text.data() + text.size(), converted,
                          std::chars_format::general);
      if (result.ec != std::errc{} || result.ptr != text.data() + text.size()) {
        throw std::invalid_argument(
            "DOUBLE PRECISION comparison value is out of range");
      }
      value = converted;
    }
    return;
  }

  if (type == SqlTypeId::Real) {
    if (const auto *doubleValue = std::get_if<double>(&value)) {
      const float realValue = static_cast<float>(*doubleValue);
      if (std::isfinite(*doubleValue) && !std::isfinite(realValue)) {
        throw std::invalid_argument("REAL comparison value is out of range");
      }
      value = realValue;
    }
  } else if (const auto *integer = std::get_if<std::int64_t>(&value)) {
    value = static_cast<double>(*integer);
  }
}

template <typename T> int compareFloatingPoint(T left, T right) {
  const bool leftIsNaN = std::isnan(left);
  const bool rightIsNaN = std::isnan(right);
  if (leftIsNaN || rightIsNaN) {
    return leftIsNaN == rightIsNaN ? 0 : (leftIsNaN ? 1 : -1);
  }
  return (left > right) - (left < right);
}

int compareValuesForColumn(
    const SqlValue &left, const SqlValue &right,
    const std::optional<unidb::sql::SqlTypeDescriptor> &declaredType) {
  if (declaredType.has_value() &&
      (declaredType->id == unidb::sql::SqlTypeId::Char ||
       declaredType->id == unidb::sql::SqlTypeId::Varchar ||
       declaredType->id == unidb::sql::SqlTypeId::Text)) {
    return compareSqlCharacterValues(*declaredType, std::get<std::string>(left),
                                     std::get<std::string>(right));
  }
  return compareSqlValues(left, right);
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
  if (const auto *leftNumeric =
          std::get_if<unidb::sql::SqlNumericValue>(&left)) {
    return compareSqlNumericValues(
        *leftNumeric, std::get<unidb::sql::SqlNumericValue>(right));
  }
  const auto &leftText = std::get<std::string>(left);
  const auto &rightText = std::get<std::string>(right);
  return (leftText > rightText) - (leftText < rightText);
}

bool valueMatchesColumn(const ColumnDefinition &column, const SqlValue &value) {
  using unidb::sql::SqlTypeId;
  if (std::holds_alternative<unidb::sql::SqlNullValue>(value)) {
    return true;
  }
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
    case SqlTypeId::Numeric:
      return std::holds_alternative<unidb::sql::SqlNumericValue>(value);
    case SqlTypeId::Char:
    case SqlTypeId::Varchar:
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

SqlTruthValue matchesWhere(
    const SqlValue &rowValue, const WhereEquals &predicate,
    const std::optional<unidb::sql::SqlTypeDescriptor> &declaredType) {
  if (predicate.comparison == ComparisonOperator::IsNull) {
    return std::holds_alternative<unidb::sql::SqlNullValue>(rowValue)
               ? SqlTruthValue::True : SqlTruthValue::False;
  }
  if (predicate.comparison == ComparisonOperator::IsNotNull) {
    return std::holds_alternative<unidb::sql::SqlNullValue>(rowValue)
               ? SqlTruthValue::False : SqlTruthValue::True;
  }
  if (std::holds_alternative<unidb::sql::SqlNullValue>(rowValue)) {
    return SqlTruthValue::Unknown;
  }
  const auto isNull = [](const SqlValue &value) {
    return std::holds_alternative<unidb::sql::SqlNullValue>(value);
  };
  if (predicate.comparison == ComparisonOperator::Like ||
      predicate.comparison == ComparisonOperator::NotLike) {
    if (isNull(predicate.value)) return SqlTruthValue::Unknown;
    const auto *textValue = std::get_if<std::string>(&rowValue);
    const auto *pattern = std::get_if<std::string>(&predicate.value);
    if (textValue == nullptr || pattern == nullptr) {
      throw std::invalid_argument("LIKE predicates require TEXT values");
    }
    const bool matched = matchesLikePattern(*textValue, *pattern);
    return (predicate.comparison == ComparisonOperator::Like) == matched
               ? SqlTruthValue::True : SqlTruthValue::False;
  }
  if (predicate.comparison == ComparisonOperator::Between ||
      predicate.comparison == ComparisonOperator::NotBetween) {
    if (!predicate.upperBound.has_value()) {
      throw std::invalid_argument("BETWEEN predicates require two range bounds");
    }
    if (isNull(predicate.value) || isNull(*predicate.upperBound)) {
      return SqlTruthValue::Unknown;
    }
    const bool withinRange =
        compareValuesForColumn(rowValue, predicate.value, declaredType) >= 0 &&
        compareValuesForColumn(rowValue, *predicate.upperBound, declaredType) <= 0;
    return (predicate.comparison == ComparisonOperator::Between) == withinRange
               ? SqlTruthValue::True : SqlTruthValue::False;
  }
  if (predicate.comparison == ComparisonOperator::In ||
      predicate.comparison == ComparisonOperator::NotIn) {
    bool found = false;
    bool hasNull = false;
    for (const auto &value : predicate.values) {
      if (isNull(value)) {
        hasNull = true;
      } else if (compareValuesForColumn(rowValue, value, declaredType) == 0) {
        found = true;
        break;
      }
    }
    if (found) {
      return predicate.comparison == ComparisonOperator::In
                 ? SqlTruthValue::True : SqlTruthValue::False;
    }
    if (hasNull) return SqlTruthValue::Unknown;
    return predicate.comparison == ComparisonOperator::In
               ? SqlTruthValue::False : SqlTruthValue::True;
  }
  if (isNull(predicate.value)) return SqlTruthValue::Unknown;

  const int comparison =
      compareValuesForColumn(rowValue, predicate.value, declaredType);
  bool matched = false;
  switch (predicate.comparison) {
  case ComparisonOperator::Equal: matched = comparison == 0; break;
  case ComparisonOperator::NotEqual: matched = comparison != 0; break;
  case ComparisonOperator::GreaterThan: matched = comparison > 0; break;
  case ComparisonOperator::LessThan: matched = comparison < 0; break;
  case ComparisonOperator::GreaterThanOrEqual: matched = comparison >= 0; break;
  case ComparisonOperator::LessThanOrEqual: matched = comparison <= 0; break;
  case ComparisonOperator::In:
  case ComparisonOperator::NotIn:
    throw std::invalid_argument("IN predicates require a value list");
  case ComparisonOperator::Between:
  case ComparisonOperator::NotBetween:
    throw std::invalid_argument("BETWEEN predicates require two range bounds");
  case ComparisonOperator::Like:
  case ComparisonOperator::NotLike:
    throw std::invalid_argument("LIKE predicates require a text pattern");
  case ComparisonOperator::IsNull:
  case ComparisonOperator::IsNotNull: break;
  }
  return matched ? SqlTruthValue::True : SqlTruthValue::False;
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
    compiled.declaredType = found->declaredType;
    auto predicate = expression.predicate;
    normalizeComparisonValue(*found, predicate.value);
    if (predicate.upperBound.has_value()) {
      normalizeComparisonValue(*found, *predicate.upperBound);
    }
    for (auto &value : predicate.values) {
      normalizeComparisonValue(*found, value);
    }
    const bool isMembership = predicate.comparison == ComparisonOperator::In ||
                              predicate.comparison == ComparisonOperator::NotIn;
    const bool isRange = predicate.comparison == ComparisonOperator::Between ||
                         predicate.comparison == ComparisonOperator::NotBetween;
    const bool isLike = predicate.comparison == ComparisonOperator::Like ||
                        predicate.comparison == ComparisonOperator::NotLike;
    const bool isNullTest =
        predicate.comparison == ComparisonOperator::IsNull ||
        predicate.comparison == ComparisonOperator::IsNotNull;
    if (isLike && types[compiled.columnIndex] != SqlType::Text) {
      throw std::invalid_argument("LIKE requires a TEXT column: " +
                                  found->name);
    }
    if (!isMembership && !isNullTest &&
        !valueMatchesColumn(*found, predicate.value)) {
      throw std::invalid_argument("WHERE value type does not match column: " +
                                  found->name);
    }
    if (isRange) {
      if (!predicate.upperBound.has_value()) {
        throw std::invalid_argument(
            "BETWEEN predicates require two range bounds");
      }
      if (!valueMatchesColumn(*found, *predicate.upperBound)) {
        throw std::invalid_argument("WHERE value type does not match column: " +
                                    found->name);
      }
    }
    if (isLike && !std::holds_alternative<std::string>(predicate.value) &&
        !std::holds_alternative<unidb::sql::SqlNullValue>(predicate.value)) {
      throw std::invalid_argument("LIKE pattern must be TEXT");
    }
    if (isMembership) {
      if (predicate.values.empty()) {
        throw std::invalid_argument("IN list must contain at least one value");
      }
      for (const auto &value : predicate.values) {
        if (!valueMatchesColumn(*found, value)) {
          throw std::invalid_argument(
              "WHERE value type does not match column: " + found->name);
        }
      }
    }
    compiled.predicate = predicate;
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

SqlTruthValue evaluateWhereExpression(
    const std::vector<SqlValue> &row,
    const CompiledWhereExpression &expression) {
  if (expression.kind == WhereExpression::Kind::Predicate) {
    return matchesWhere(row[expression.columnIndex], expression.predicate,
                        expression.declaredType);
  }
  bool hasUnknown = false;
  for (const auto &child : expression.children) {
    const auto value = evaluateWhereExpression(row, child);
    if (expression.kind == WhereExpression::Kind::And &&
        value == SqlTruthValue::False) return SqlTruthValue::False;
    if (expression.kind == WhereExpression::Kind::Or &&
        value == SqlTruthValue::True) return SqlTruthValue::True;
    hasUnknown = hasUnknown || value == SqlTruthValue::Unknown;
  }
  if (hasUnknown) return SqlTruthValue::Unknown;
  return expression.kind == WhereExpression::Kind::And
             ? SqlTruthValue::True : SqlTruthValue::False;
}

bool matchesWhereExpression(const std::vector<SqlValue> &row,
                            const CompiledWhereExpression &expression) {
  return evaluateWhereExpression(row, expression) == SqlTruthValue::True;
}

} // namespace QueryExecutionService::domain
