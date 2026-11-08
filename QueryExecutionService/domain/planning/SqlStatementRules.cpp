#include "SqlStatementRules.h"

#include "../schema/SqlSchemaResolver.h"
#include "../codec/SqlTupleCodec.h"

#include <algorithm>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>

namespace QueryExecutionService::domain {

using namespace unidb::sql;

void validateCreateTable(const CreateTableStatement &statement) {
  if (statement.columns.empty()) {
    throw std::invalid_argument("CREATE TABLE requires at least one column");
  }
  std::unordered_set<std::string> names;
  for (const auto &column : statement.columns) {
    if (column.name.empty() || !names.insert(column.name).second) {
      throw std::invalid_argument("Column names must be non-empty and unique");
    }
    if (column.type != SqlType::Int && column.type != SqlType::Text) {
      throw std::invalid_argument("Column type is outside the V1 SQL subset");
    }
  }
}

std::vector<std::vector<SqlValue>>
prepareInsertRows(const InsertStatement &statement,
                  const std::vector<ColumnDefinition> &schema) {
  const auto types = schemaTypes(schema);
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
      const bool valueIsInt =
          std::holds_alternative<std::int64_t>(values[inputIndex]);
      if ((types[schemaIndex] == SqlType::Int) != valueIsInt) {
        throw std::invalid_argument(
            "INSERT value type does not match column: " + found->name);
      }
      orderedValues[schemaIndex] = values[inputIndex];
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
  const auto types = schemaTypes(schema);
  const auto rows = prepareInsertRows(statement, schema);
  std::vector<std::vector<char>> encodedRows;
  encodedRows.reserve(rows.size());
  for (const auto &row : rows) {
    encodedRows.push_back(SqlTupleCodec::encode(types, row));
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
  std::set<std::vector<SqlValue>> seenRows;
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
      if (branch.columns[index].type != combined.columns[index].type) {
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
