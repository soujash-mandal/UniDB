#include "SqlStatementRules.h"

#include "../codec/SqlTupleCodec.h"
#include "../predicate/WherePredicateEvaluator.h"
#include "../types/SqlNumericSemantics.h"

#include <charconv>
#include <algorithm>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>

namespace QueryExecutionService::domain {

using namespace unidb::sql;

namespace {
struct SqlValueRowLess {
  bool operator()(const std::vector<SqlValue> &left,
                  const std::vector<SqlValue> &right) const {
    return std::lexicographical_compare(
        left.begin(), left.end(), right.begin(), right.end(),
        [](const SqlValue &leftValue, const SqlValue &rightValue) {
          if (leftValue.index() != rightValue.index()) {
            return leftValue.index() < rightValue.index();
          }
          return compareSqlValues(leftValue, rightValue) < 0;
        });
  }
};
} // namespace

void validateCreateTable(const CreateTableStatement &statement) {
  if (statement.columns.empty()) {
    throw std::invalid_argument("CREATE TABLE requires at least one column");
  }
  std::unordered_set<std::string> names;
  for (const auto &column : statement.columns) {
    if (column.name.empty() || !names.insert(column.name).second) {
      throw std::invalid_argument("Column names must be non-empty and unique");
    }
    if (column.declaredType.has_value()) {
      using unidb::sql::SqlTypeId;
      const auto id = column.declaredType->id;
      if (id != SqlTypeId::SmallInt && id != SqlTypeId::Integer &&
          id != SqlTypeId::BigInt && id != SqlTypeId::Real &&
          id != SqlTypeId::DoublePrecision && id != SqlTypeId::Numeric &&
          id != SqlTypeId::Boolean && id != SqlTypeId::Text) {
        throw std::invalid_argument(
            "CREATE TABLE type is recognized but not yet supported");
      }
    } else if (column.type != SqlType::Int && column.type != SqlType::Text) {
      throw std::invalid_argument("Column type is outside the V1 SQL subset");
    }
  }
}

std::vector<std::vector<SqlValue>>
prepareInsertRows(const InsertStatement &statement,
                  const std::vector<ColumnDefinition> &schema) {
  if (statement.rows.empty()) {
    throw std::invalid_argument("INSERT requires at least one VALUES row");
  }
  std::vector<std::vector<SqlValue>> orderedRows;
  orderedRows.reserve(statement.rows.size());
  for (const auto &values : statement.rows) {
    if (statement.columnNames.size() != values.size()) {
      throw std::invalid_argument("INSERT column and value counts must match");
    }
    std::vector<SqlValue> orderedValues(schema.size());
    std::vector<bool> assigned(schema.size(), false);
    for (std::size_t inputIndex = 0; inputIndex < statement.columnNames.size();
         ++inputIndex) {
      const auto found =
          std::find_if(schema.begin(), schema.end(), [&](const auto &column) {
            return column.name == statement.columnNames[inputIndex];
          });
      if (found == schema.end()) {
        throw std::invalid_argument("Unknown INSERT column: " +
                                    statement.columnNames[inputIndex]);
      }
      const auto schemaIndex =
          static_cast<std::size_t>(std::distance(schema.begin(), found));
      if (assigned[schemaIndex]) {
        throw std::invalid_argument("Duplicate INSERT column: " +
                                    statement.columnNames[inputIndex]);
      }
      bool valueMatchesColumn = false;
      bool valueWasNormalized = false;
      if (found->declaredType.has_value()) {
        using unidb::sql::SqlTypeId;
        switch (found->declaredType->id) {
        case SqlTypeId::SmallInt:
        case SqlTypeId::Integer:
        case SqlTypeId::BigInt:
          valueMatchesColumn =
              std::holds_alternative<std::int64_t>(values[inputIndex]);
          break;
        case SqlTypeId::Real: {
          double numericValue{};
          if (const auto *floating = std::get_if<double>(&values[inputIndex])) {
            numericValue = *floating;
          } else if (const auto *integer =
                         std::get_if<std::int64_t>(&values[inputIndex])) {
            numericValue = static_cast<double>(*integer);
          } else if (const auto *decimal =
                         std::get_if<SqlNumericValue>(&values[inputIndex])) {
            const auto text = formatSqlNumericValue(*decimal);
            const auto conversion = std::from_chars(
                text.data(), text.data() + text.size(), numericValue,
                std::chars_format::general);
            if (conversion.ec != std::errc{} ||
                conversion.ptr != text.data() + text.size()) {
              throw std::invalid_argument(
                  "REAL value is outside the supported range: " + found->name);
            }
          } else {
            break;
          }
          if (numericValue > std::numeric_limits<float>::max() ||
              numericValue < std::numeric_limits<float>::lowest()) {
            throw std::invalid_argument(
                "REAL value is outside the supported range: " + found->name);
          }
          orderedValues[schemaIndex] = static_cast<float>(numericValue);
          valueWasNormalized = true;
          valueMatchesColumn = true;
          break;
        }
        case SqlTypeId::DoublePrecision:
          if (const auto *floating = std::get_if<double>(&values[inputIndex])) {
            orderedValues[schemaIndex] = *floating;
            valueWasNormalized = true;
            valueMatchesColumn = true;
          } else if (const auto *integer =
                         std::get_if<std::int64_t>(&values[inputIndex])) {
            orderedValues[schemaIndex] = static_cast<double>(*integer);
            valueWasNormalized = true;
            valueMatchesColumn = true;
          } else if (const auto *decimal =
                         std::get_if<SqlNumericValue>(&values[inputIndex])) {
            const auto text = formatSqlNumericValue(*decimal);
            double converted{};
            const auto conversion = std::from_chars(
                text.data(), text.data() + text.size(), converted,
                std::chars_format::general);
            if (conversion.ec != std::errc{} ||
                conversion.ptr != text.data() + text.size()) {
              throw std::invalid_argument(
                  "DOUBLE PRECISION value is outside the supported range: " +
                  found->name);
            }
            orderedValues[schemaIndex] = converted;
            valueWasNormalized = true;
            valueMatchesColumn = true;
          }
          break;
        case SqlTypeId::Numeric:
          if (std::holds_alternative<SqlNumericValue>(values[inputIndex])) {
            valueMatchesColumn = true;
          } else if (const auto *integer =
                         std::get_if<std::int64_t>(&values[inputIndex])) {
            orderedValues[schemaIndex] =
                parseSqlNumericLiteral(std::to_string(*integer));
            valueWasNormalized = true;
            valueMatchesColumn = true;
          }
          break;
        case SqlTypeId::Boolean:
          valueMatchesColumn = std::holds_alternative<bool>(values[inputIndex]);
          break;
        case SqlTypeId::Text:
          valueMatchesColumn =
              std::holds_alternative<std::string>(values[inputIndex]);
          break;
        default:
          throw std::invalid_argument(
              "INSERT column type is recognized but not yet supported: " +
              found->name);
        }
      } else if (found->type == SqlType::Int) {
        valueMatchesColumn =
            std::holds_alternative<std::int64_t>(values[inputIndex]);
      } else if (found->type == SqlType::Text) {
        valueMatchesColumn =
            std::holds_alternative<std::string>(values[inputIndex]);
      }
      if (!valueMatchesColumn) {
        throw std::invalid_argument(
            "INSERT value type does not match column: " + found->name);
      }
      if (!valueWasNormalized) {
        orderedValues[schemaIndex] = values[inputIndex];
      }
      assigned[schemaIndex] = true;
    }
    if (std::find(assigned.begin(), assigned.end(), false) != assigned.end()) {
      throw std::invalid_argument(
          "V1 INSERT requires a value for every table column");
    }
    orderedRows.push_back(std::move(orderedValues));
  }
  return orderedRows;
}

std::vector<std::vector<char>>
encodeInsertRows(const InsertStatement &statement,
                 const std::vector<ColumnDefinition> &schema) {
  const auto rows = prepareInsertRows(statement, schema);
  std::vector<std::vector<char>> encodedRows;
  encodedRows.reserve(rows.size());
  for (const auto &row : rows) {
    encodedRows.push_back(SqlTupleCodec::encode(schema, row));
  }
  return encodedRows;
}

QueryExecutionResult
combineUnionResults(const std::vector<QueryExecutionResult> &branches,
                    bool removeDuplicates) {
  if (branches.size() < 2) {
    throw std::invalid_argument(
        removeDuplicates ? "UNION requires at least two SELECT branches"
                         : "UNION ALL requires at least two SELECT branches");
  }
  QueryExecutionResult combined;
  combined.kind = QueryExecutionResultKind::Rows;
  combined.message = "Rows selected";
  combined.columns = branches.front().columns;
  std::set<std::vector<SqlValue>, SqlValueRowLess> seenRows;
  for (const auto &branch : branches) {
    if (branch.kind != QueryExecutionResultKind::Rows) {
      throw std::invalid_argument(removeDuplicates
                                      ? "UNION branches must return rows"
                                      : "UNION ALL branches must return rows");
    }
    if (branch.columns.size() != combined.columns.size()) {
      throw std::invalid_argument(
          removeDuplicates
              ? "UNION branches must return the same number of columns"
              : "UNION ALL branches must return the same number of columns");
    }
    for (std::size_t index = 0; index < combined.columns.size(); ++index) {
      const auto &branchColumn = branch.columns[index];
      const auto &combinedColumn = combined.columns[index];
      const bool declaredTypesMatch =
          branchColumn.declaredType == combinedColumn.declaredType;
      if (branchColumn.type != combinedColumn.type || !declaredTypesMatch) {
        throw std::invalid_argument(
            removeDuplicates
                ? "UNION columns must have matching types at each position"
                : "UNION ALL columns must have matching types at each "
                  "position");
      }
    }
    if (removeDuplicates) {
      for (const auto &row : branch.rows) {
        if (seenRows.insert(row).second) {
          combined.rows.push_back(row);
        }
      }
    } else {
      combined.rows.insert(combined.rows.end(), branch.rows.begin(),
                           branch.rows.end());
    }
  }
  return combined;
}

} // namespace QueryExecutionService::domain
