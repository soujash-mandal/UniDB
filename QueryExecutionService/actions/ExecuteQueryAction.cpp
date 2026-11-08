#include "ExecuteQueryAction.h"

#include "../domain/aggregate/SqlAggregateCalculator.h"
#include "../domain/codec/SqlTupleCodec.h"
#include "../domain/expression/ScalarExpressionEvaluator.h"
#include "../domain/expression/SqlExpressionCompiler.h"
#include "../domain/join/JoinValidator.h"
#include "../domain/predicate/WhereExpressionNormalizer.h"
#include "../domain/predicate/WherePredicateEvaluator.h"
#include "../domain/schema/SqlSchemaResolver.h"
#include "../domain/planning/SqlQueryPlanner.h"

#include <algorithm>
#include <functional>
#include <optional>
#include <stdexcept>
#include <utility>

namespace QueryExecutionService {
namespace {

using unidb::sql::AggregateFunction;
using unidb::sql::ColumnDefinition;
using unidb::sql::ComparisonOperator;
using unidb::sql::CreateTableStatement;
using unidb::sql::InsertStatement;
using unidb::sql::SelectStatement;
using unidb::sql::SqlStatement;
using unidb::sql::SqlType;
using unidb::sql::SqlValue;
using unidb::sql::TextFunction;
using unidb::sql::UnionAllStatement;
using unidb::sql::UnionStatement;
using unidb::sql::WhereExpression;

QueryExecutionError executionError(std::string message) {
  return {QueryExecutionErrorCode::ExecutionError, std::move(message)};
}

QueryExecutionResult
executeInnerJoin(RdbServicePort &rdbServicePort, const SelectStatement &select,
                 const std::vector<ColumnDefinition> &leftSchema,
                 const std::vector<ColumnDefinition> &rightSchema) {
  const auto &join = select.innerJoins.front();
  const std::string leftQualifier =
      select.tableAlias.value_or(select.tableName);
  const std::string rightQualifier = join.tableAlias.value_or(join.tableName);
  const auto leftTypes = domain::schemaTypes(leftSchema);
  const auto rightTypes = domain::schemaTypes(rightSchema);
  const auto leftKey = domain::resolveJoinColumn(
      join.left, leftQualifier, leftSchema, rightQualifier, rightSchema);
  const auto rightKey = domain::resolveJoinColumn(
      join.right, leftQualifier, leftSchema, rightQualifier, rightSchema);
  const std::size_t leftKeyIndex =
      leftKey.tableName == leftQualifier ? leftKey.index : rightKey.index;
  const std::size_t rightKeyIndex =
      leftKey.tableName == rightQualifier ? leftKey.index : rightKey.index;

  // Use fully-qualified internal names so WHERE can resolve either qualified
  // references or unqualified names after ambiguity checks.
  std::vector<ColumnDefinition> joinedSchema;
  std::vector<SqlType> joinedTypes;
  joinedSchema.reserve(leftSchema.size() + rightSchema.size());
  joinedTypes.reserve(leftTypes.size() + rightTypes.size());
  for (const auto &column : leftSchema) {
    joinedSchema.push_back({leftQualifier + "." + column.name, column.type});
    joinedTypes.push_back(column.type);
  }
  for (const auto &column : rightSchema) {
    joinedSchema.push_back({rightQualifier + "." + column.name, column.type});
    joinedTypes.push_back(column.type);
  }

  std::optional<domain::CompiledWhereExpression> compiledWhere;
  if (const auto *where = std::get_if<WhereExpression>(&select.where)) {
    const auto qualified = domain::qualifyJoinWhereColumns(
        *where, leftQualifier, leftSchema, rightQualifier, rightSchema);
    compiledWhere =
        domain::compileWhereExpression(qualified, joinedSchema, joinedTypes);
  }

  std::vector<std::size_t> selectedIndexes;
  std::vector<ColumnDefinition> selectedColumns;
  if (select.selectAll) {
    selectedIndexes.reserve(joinedSchema.size());
    selectedColumns.reserve(joinedSchema.size());
    for (std::size_t index = 0; index < joinedSchema.size(); ++index) {
      selectedIndexes.push_back(index);
      const bool isLeft = index < leftSchema.size();
      const auto &sourceColumn =
          isLeft ? leftSchema[index] : rightSchema[index - leftSchema.size()];
      const auto &otherSchema = isLeft ? rightSchema : leftSchema;
      const bool duplicated = std::any_of(
          otherSchema.begin(), otherSchema.end(),
          [&](const auto &other) { return other.name == sourceColumn.name; });
      const std::string &tableName = isLeft ? leftQualifier : rightQualifier;
      selectedColumns.push_back(
          {duplicated ? tableName + "." + sourceColumn.name : sourceColumn.name,
           sourceColumn.type});
    }
  } else {
    selectedIndexes.reserve(select.columnNames.size());
    selectedColumns.reserve(select.columnNames.size());
    for (std::size_t index = 0; index < select.columnNames.size(); ++index) {
      const auto resolved = domain::resolveSelectedJoinColumn(
          select.columnNames[index], leftQualifier, leftSchema, rightQualifier,
          rightSchema);
      selectedIndexes.push_back(resolved.tableName == leftQualifier
                                    ? resolved.index
                                    : leftSchema.size() + resolved.index);
      const bool duplicated =
          std::any_of(leftSchema.begin(), leftSchema.end(),
                      [&](const auto &column) {
                        return column.name == resolved.column->name;
                      }) &&
          std::any_of(rightSchema.begin(), rightSchema.end(),
                      [&](const auto &column) {
                        return column.name == resolved.column->name;
                      });
      std::string outputName =
          duplicated ? resolved.tableName + "." + resolved.column->name
                     : resolved.column->name;
      if (index < select.columnAliases.size() &&
          select.columnAliases[index].has_value()) {
        outputName = *select.columnAliases[index];
      }
      selectedColumns.push_back({std::move(outputName), resolved.column->type});
    }
  }

  const auto leftEncodedRows = rdbServicePort.selectAllRows(select.tableName);
  const auto rightEncodedRows = rdbServicePort.selectAllRows(join.tableName);
  std::vector<std::vector<SqlValue>> leftRows;
  leftRows.reserve(leftEncodedRows.size());
  for (const auto &bytes : leftEncodedRows) {
    leftRows.push_back(SqlTupleCodec::decode(leftTypes, bytes));
  }
  std::vector<std::vector<SqlValue>> rightRows;
  rightRows.reserve(rightEncodedRows.size());
  for (const auto &bytes : rightEncodedRows) {
    rightRows.push_back(SqlTupleCodec::decode(rightTypes, bytes));
  }

  QueryExecutionResult result;
  result.kind = QueryExecutionResultKind::Rows;
  result.message = "Rows selected";
  result.columns = std::move(selectedColumns);
  for (const auto &leftRow : leftRows) {
    for (const auto &rightRow : rightRows) {
      const int comparison = domain::compareSqlValues(leftRow[leftKeyIndex],
                                                      rightRow[rightKeyIndex]);
      bool matches = false;
      switch (join.comparison) {
      case ComparisonOperator::Equal:
        matches = comparison == 0;
        break;
      case ComparisonOperator::NotEqual:
        matches = comparison != 0;
        break;
      case ComparisonOperator::GreaterThan:
        matches = comparison > 0;
        break;
      case ComparisonOperator::LessThan:
        matches = comparison < 0;
        break;
      case ComparisonOperator::GreaterThanOrEqual:
        matches = comparison >= 0;
        break;
      case ComparisonOperator::LessThanOrEqual:
        matches = comparison <= 0;
        break;
      default:
        throw std::invalid_argument(
            "INNER JOIN supports only basic comparisons");
      }
      if (!matches) {
        continue;
      }
      std::vector<SqlValue> joinedRow;
      joinedRow.reserve(leftRow.size() + rightRow.size());
      joinedRow.insert(joinedRow.end(), leftRow.begin(), leftRow.end());
      joinedRow.insert(joinedRow.end(), rightRow.begin(), rightRow.end());
      if (compiledWhere.has_value() &&
          !domain::matchesWhereExpression(joinedRow, *compiledWhere)) {
        continue;
      }
      std::vector<SqlValue> projectedRow;
      projectedRow.reserve(selectedIndexes.size());
      for (const auto index : selectedIndexes) {
        projectedRow.push_back(joinedRow[index]);
      }
      result.rows.push_back(std::move(projectedRow));
    }
  }
  return result;
}

QueryExecutionResult
executeCrossJoin(RdbServicePort &rdbServicePort, const SelectStatement &select,
                 const std::vector<ColumnDefinition> &leftSchema,
                 const std::vector<ColumnDefinition> &rightSchema) {
  const auto &join = *select.crossJoin;
  const std::string leftQualifier =
      select.tableAlias.value_or(select.tableName);
  const std::string rightQualifier = join.tableAlias.value_or(join.tableName);
  if (leftQualifier == rightQualifier ||
      (select.tableName == join.tableName && !select.tableAlias &&
       !join.tableAlias)) {
    throw std::invalid_argument(
        "CROSS JOIN of the same table requires distinct aliases");
  }
  if (select.aggregate || !select.groupBy.empty() || select.having ||
      select.distinct || !select.orderBy.empty() || select.limit ||
      select.offset != 0 ||
      !std::all_of(select.integerExpressions.begin(),
                   select.integerExpressions.end(),
                   [](const auto &value) { return !value.has_value(); }) ||
      !std::all_of(select.textFunctions.begin(), select.textFunctions.end(),
                   [](const auto &value) { return !value.has_value(); }) ||
      !std::all_of(select.caseExpressions.begin(), select.caseExpressions.end(),
                   [](const auto &value) { return !value.has_value(); })) {
    throw std::invalid_argument(
        "CROSS JOIN currently supports simple SELECT projections and WHERE");
  }

  const auto leftTypes = domain::schemaTypes(leftSchema);
  const auto rightTypes = domain::schemaTypes(rightSchema);
  // Use fully-qualified internal names so WHERE can resolve either qualified
  // references or unqualified names after ambiguity checks.
  std::vector<ColumnDefinition> joinedSchema;
  std::vector<SqlType> joinedTypes;
  joinedSchema.reserve(leftSchema.size() + rightSchema.size());
  joinedTypes.reserve(leftTypes.size() + rightTypes.size());
  for (const auto &column : leftSchema) {
    joinedSchema.push_back({leftQualifier + "." + column.name, column.type});
    joinedTypes.push_back(column.type);
  }
  for (const auto &column : rightSchema) {
    joinedSchema.push_back({rightQualifier + "." + column.name, column.type});
    joinedTypes.push_back(column.type);
  }

  std::optional<domain::CompiledWhereExpression> compiledWhere;
  if (const auto *where = std::get_if<WhereExpression>(&select.where)) {
    const auto qualified = domain::qualifyJoinWhereColumns(
        *where, leftQualifier, leftSchema, rightQualifier, rightSchema);
    compiledWhere =
        domain::compileWhereExpression(qualified, joinedSchema, joinedTypes);
  }

  std::vector<std::size_t> selectedIndexes;
  std::vector<ColumnDefinition> selectedColumns;
  if (select.selectAll) {
    selectedIndexes.reserve(joinedSchema.size());
    selectedColumns.reserve(joinedSchema.size());
    for (std::size_t index = 0; index < joinedSchema.size(); ++index) {
      selectedIndexes.push_back(index);
      const bool isLeft = index < leftSchema.size();
      const auto &sourceColumn =
          isLeft ? leftSchema[index] : rightSchema[index - leftSchema.size()];
      const auto &otherSchema = isLeft ? rightSchema : leftSchema;
      const bool duplicated = std::any_of(
          otherSchema.begin(), otherSchema.end(),
          [&](const auto &other) { return other.name == sourceColumn.name; });
      const std::string &tableName = isLeft ? leftQualifier : rightQualifier;
      selectedColumns.push_back(
          {duplicated ? tableName + "." + sourceColumn.name : sourceColumn.name,
           sourceColumn.type});
    }
  } else {
    selectedIndexes.reserve(select.columnNames.size());
    selectedColumns.reserve(select.columnNames.size());
    for (std::size_t index = 0; index < select.columnNames.size(); ++index) {
      const auto resolved = domain::resolveSelectedJoinColumn(
          select.columnNames[index], leftQualifier, leftSchema, rightQualifier,
          rightSchema);
      selectedIndexes.push_back(resolved.tableName == leftQualifier
                                    ? resolved.index
                                    : leftSchema.size() + resolved.index);
      const bool duplicated =
          std::any_of(leftSchema.begin(), leftSchema.end(),
                      [&](const auto &column) {
                        return column.name == resolved.column->name;
                      }) &&
          std::any_of(rightSchema.begin(), rightSchema.end(),
                      [&](const auto &column) {
                        return column.name == resolved.column->name;
                      });
      std::string outputName =
          duplicated ? resolved.tableName + "." + resolved.column->name
                     : resolved.column->name;
      if (index < select.columnAliases.size() &&
          select.columnAliases[index].has_value()) {
        outputName = *select.columnAliases[index];
      }
      selectedColumns.push_back({std::move(outputName), resolved.column->type});
    }
  }

  const auto leftEncodedRows = rdbServicePort.selectAllRows(select.tableName);
  const auto rightEncodedRows = rdbServicePort.selectAllRows(join.tableName);
  std::vector<std::vector<SqlValue>> leftRows;
  leftRows.reserve(leftEncodedRows.size());
  for (const auto &bytes : leftEncodedRows) {
    leftRows.push_back(SqlTupleCodec::decode(leftTypes, bytes));
  }
  std::vector<std::vector<SqlValue>> rightRows;
  rightRows.reserve(rightEncodedRows.size());
  for (const auto &bytes : rightEncodedRows) {
    rightRows.push_back(SqlTupleCodec::decode(rightTypes, bytes));
  }
  constexpr std::size_t kMaxCrossJoinRows = 1'000'000;
  if (!leftRows.empty() &&
      rightRows.size() > kMaxCrossJoinRows / leftRows.size()) {
    throw std::length_error(
        "CROSS JOIN result exceeds the 1000000-row safety limit");
  }

  QueryExecutionResult result;
  result.kind = QueryExecutionResultKind::Rows;
  result.message = "Rows selected";
  result.columns = std::move(selectedColumns);
  for (const auto &leftRow : leftRows) {
    for (const auto &rightRow : rightRows) {
      std::vector<SqlValue> joinedRow;
      joinedRow.reserve(leftRow.size() + rightRow.size());
      joinedRow.insert(joinedRow.end(), leftRow.begin(), leftRow.end());
      joinedRow.insert(joinedRow.end(), rightRow.begin(), rightRow.end());
      if (compiledWhere.has_value() &&
          !domain::matchesWhereExpression(joinedRow, *compiledWhere)) {
        continue;
      }
      std::vector<SqlValue> projectedRow;
      projectedRow.reserve(selectedIndexes.size());
      for (const auto index : selectedIndexes) {
        projectedRow.push_back(joinedRow[index]);
      }
      result.rows.push_back(std::move(projectedRow));
    }
  }
  return result;
}

QueryExecutionResult executeMultipleInnerJoins(
    RdbServicePort &port, const SelectStatement &select,
    const std::vector<std::vector<ColumnDefinition>> &schemas) {
  if (select.innerJoins.size() < 2 ||
      schemas.size() != select.innerJoins.size() + 1)
    throw std::invalid_argument("Invalid multiple INNER JOIN plan");
  if (select.aggregate || !select.groupBy.empty() || select.having ||
      select.distinct || !select.orderBy.empty() || select.limit ||
      select.offset != 0 ||
      !std::all_of(select.integerExpressions.begin(),
                   select.integerExpressions.end(),
                   [](const auto &v) { return !v.has_value(); }) ||
      !std::all_of(select.textFunctions.begin(), select.textFunctions.end(),
                   [](const auto &v) { return !v.has_value(); }) ||
      !std::all_of(select.caseExpressions.begin(), select.caseExpressions.end(),
                   [](const auto &v) { return !v.has_value(); }))
    throw std::invalid_argument(
        "Multiple INNER JOIN currently supports simple SELECT projections");
  std::vector<std::string> qualifiers;
  qualifiers.push_back(select.tableAlias.value_or(select.tableName));
  std::vector<ColumnDefinition> combinedSchema;
  std::vector<SqlType> combinedTypes;
  std::vector<std::vector<SqlValue>> rows;
  auto firstTypes = domain::schemaTypes(schemas.front());
  for (const auto &c : schemas.front()) {
    combinedSchema.push_back({qualifiers.front() + "." + c.name, c.type});
    combinedTypes.push_back(c.type);
  }
  for (const auto &bytes : port.selectAllRows(select.tableName))
    rows.push_back(SqlTupleCodec::decode(firstTypes, bytes));
  for (std::size_t j = 0; j < select.innerJoins.size(); ++j) {
    const auto &join = select.innerJoins[j];
    const auto qualifier = join.tableAlias.value_or(join.tableName);
    if (std::find(qualifiers.begin(), qualifiers.end(), qualifier) !=
        qualifiers.end())
      throw std::invalid_argument("JOIN table aliases must be distinct");
    if (std::find_if(qualifiers.begin(), qualifiers.end(),
                     [&](const auto &q) { return q == join.tableName; }) !=
            qualifiers.end() &&
        !join.tableAlias)
      throw std::invalid_argument(
          "Joining a table to itself requires a distinct alias");
    auto resolve =
        [&](const unidb::sql::ColumnReference &ref,
            bool allowNew) -> std::pair<std::size_t, const ColumnDefinition *> {
      if (!ref.tableName)
        throw std::invalid_argument(
            "JOIN ON requires table-qualified column references");
      if (*ref.tableName == qualifier && allowNew) {
        const auto &sc = schemas[j + 1];
        auto it = std::find_if(sc.begin(), sc.end(), [&](const auto &c) {
          return c.name == ref.columnName;
        });
        if (it == sc.end())
          throw std::invalid_argument("Unknown JOIN column: " + *ref.tableName +
                                      "." + ref.columnName);
        return {static_cast<std::size_t>(std::distance(sc.begin(), it)), &*it};
      }
      auto q = std::find(qualifiers.begin(), qualifiers.end(), *ref.tableName);
      if (q == qualifiers.end())
        throw std::invalid_argument("Unknown JOIN table reference: " +
                                    *ref.tableName);
      const auto qi =
          static_cast<std::size_t>(std::distance(qualifiers.begin(), q));
      std::size_t offset = 0;
      for (std::size_t k = 0; k < qi; ++k)
        offset += schemas[k].size();
      const auto &sc = schemas[qi];
      auto it = std::find_if(sc.begin(), sc.end(), [&](const auto &c) {
        return c.name == ref.columnName;
      });
      if (it == sc.end())
        throw std::invalid_argument("Unknown JOIN column: " + *ref.tableName +
                                    "." + ref.columnName);
      return {offset + static_cast<std::size_t>(std::distance(sc.begin(), it)),
              &*it};
    };
    auto l = resolve(join.left, true), rr = resolve(join.right, true);
    const bool lnew = join.left.tableName && *join.left.tableName == qualifier;
    const bool rnew =
        join.right.tableName && *join.right.tableName == qualifier;
    if (lnew == rnew)
      throw std::invalid_argument("INNER JOIN ON must compare a previously "
                                  "joined table with the new table");
    if (l.second->type != rr.second->type)
      throw std::invalid_argument(
          "INNER JOIN columns must have matching types");
    const std::size_t priorIndex = lnew ? rr.first : l.first;
    const std::size_t newIndex = lnew ? l.first : rr.first;
    const auto newTypes = domain::schemaTypes(schemas[j + 1]);
    std::vector<std::vector<SqlValue>> newRows;
    for (const auto &bytes : port.selectAllRows(join.tableName))
      newRows.push_back(SqlTupleCodec::decode(newTypes, bytes));
    std::vector<std::vector<SqlValue>> expanded;
    for (const auto &oldRow : rows)
      for (const auto &newRow : newRows) {
        const auto cmp = lnew ? domain::compareSqlValues(newRow[newIndex],
                                                         oldRow[priorIndex])
                              : domain::compareSqlValues(oldRow[priorIndex],
                                                         newRow[newIndex]);
        bool ok = false;
        switch (join.comparison) {
        case ComparisonOperator::Equal:
          ok = cmp == 0;
          break;
        case ComparisonOperator::NotEqual:
          ok = cmp != 0;
          break;
        case ComparisonOperator::GreaterThan:
          ok = cmp > 0;
          break;
        case ComparisonOperator::LessThan:
          ok = cmp < 0;
          break;
        case ComparisonOperator::GreaterThanOrEqual:
          ok = cmp >= 0;
          break;
        case ComparisonOperator::LessThanOrEqual:
          ok = cmp <= 0;
          break;
        default:
          throw std::invalid_argument(
              "INNER JOIN supports basic comparisons only");
        }
        if (ok) {
          auto row = oldRow;
          row.insert(row.end(), newRow.begin(), newRow.end());
          expanded.push_back(std::move(row));
        }
      }
    rows = std::move(expanded);
    qualifiers.push_back(qualifier);
    for (const auto &c : schemas[j + 1]) {
      combinedSchema.push_back({qualifier + "." + c.name, c.type});
      combinedTypes.push_back(c.type);
    }
  }
  std::optional<domain::CompiledWhereExpression> where;
  if (const auto *w = std::get_if<WhereExpression>(&select.where)) {
    auto q = *w;
    std::function<void(WhereExpression &)> qualify = [&](WhereExpression &e) {
      if (e.kind != WhereExpression::Kind::Predicate) {
        for (auto &c : e.children)
          qualify(c);
        return;
      }
      const auto name = e.predicate.columnName;
      const auto dot = name.find('.');
      if (dot != std::string::npos) {
        const auto alias = name.substr(0, dot), col = name.substr(dot + 1);
        if (std::find(qualifiers.begin(), qualifiers.end(), alias) ==
            qualifiers.end())
          throw std::invalid_argument("Unknown WHERE table reference: " +
                                      alias);
        e.predicate.columnName = alias + "." + col;
        return;
      }
      std::optional<std::string> found;
      for (std::size_t t = 0; t < schemas.size(); ++t)
        for (const auto &c : schemas[t])
          if (c.name == name) {
            if (found)
              throw std::invalid_argument("Ambiguous WHERE column: " + name);
            found = qualifiers[t] + "." + name;
          }
      if (!found)
        throw std::invalid_argument("Unknown WHERE column: " + name);
      e.predicate.columnName = *found;
    };
    qualify(q);
    where = domain::compileWhereExpression(q, combinedSchema, combinedTypes);
  }
  std::vector<std::size_t> projection;
  std::vector<ColumnDefinition> output;
  if (select.selectAll) {
    for (std::size_t i = 0; i < combinedSchema.size(); ++i) {
      projection.push_back(i);
      const auto &name = combinedSchema[i].name;
      const auto dot = name.find('.');
      const auto col = name.substr(dot + 1);
      std::size_t count = 0;
      for (const auto &sc : schemas)
        for (const auto &c : sc)
          if (c.name == col)
            ++count;
      output.push_back({count > 1 ? name : col, combinedSchema[i].type});
    }
  } else {
    if (select.columnNames.empty())
      throw std::invalid_argument("SELECT requires at least one column");
    for (std::size_t i = 0; i < select.columnNames.size(); ++i) {
      const auto name = select.columnNames[i];
      const auto dot = name.find('.');
      std::optional<std::size_t> idx;
      if (dot != std::string::npos) {
        const auto q = name.substr(0, dot), col = name.substr(dot + 1);
        auto qi = std::find(qualifiers.begin(), qualifiers.end(), q);
        if (qi == qualifiers.end())
          throw std::invalid_argument("Unknown SELECT table: " + q);
        auto ti =
            static_cast<std::size_t>(std::distance(qualifiers.begin(), qi));
        std::size_t off = 0;
        for (std::size_t k = 0; k < ti; ++k)
          off += schemas[k].size();
        for (std::size_t k = 0; k < schemas[ti].size(); ++k)
          if (schemas[ti][k].name == col)
            idx = off + k;
      } else {
        for (std::size_t k = 0; k < combinedSchema.size(); ++k)
          if (combinedSchema[k].name.substr(combinedSchema[k].name.find('.') +
                                            1) == name) {
            if (idx)
              throw std::invalid_argument("Ambiguous SELECT column: " + name);
            idx = k;
          }
      }
      if (!idx)
        throw std::invalid_argument("Unknown SELECT column: " + name);
      projection.push_back(*idx);
      auto outName = combinedSchema[*idx].name;
      const auto d = outName.find('.');
      const auto col = outName.substr(d + 1);
      std::size_t count = 0;
      for (const auto &sc : schemas)
        for (const auto &c : sc)
          if (c.name == col)
            ++count;
      if (i < select.columnAliases.size() && select.columnAliases[i])
        outName = *select.columnAliases[i];
      else if (count == 1)
        outName = col;
      output.push_back({outName, combinedSchema[*idx].type});
    }
  }
  QueryExecutionResult result;
  result.kind = QueryExecutionResultKind::Rows;
  result.message = "Rows selected";
  result.columns = std::move(output);
  for (auto &row : rows) {
    if (where && !domain::matchesWhereExpression(row, *where))
      continue;
    std::vector<SqlValue> projected;
    for (auto index : projection)
      projected.push_back(row[index]);
    result.rows.push_back(std::move(projected));
  }
  return result;
}

QueryExecutionResult commandResult(std::string message,
                                   uint32_t affectedRows = 0) {
  QueryExecutionResult result;
  result.kind = QueryExecutionResultKind::Command;
  result.message = std::move(message);
  result.affectedRows = affectedRows;
  return result;
}

} // namespace

ExecuteQueryAction::ExecuteQueryAction(RdbServicePort &rdbServicePort,
                                       const unidb::sql::SqlParser &sqlParser)
    : rdbServicePort(rdbServicePort), sqlParser(sqlParser) {}

QueryExecutionOutcome ExecuteQueryAction::execute(std::string_view sql) const {
  const unidb::sql::SqlParseResult parseResult = sqlParser.parse(sql);
  if (const auto *error =
          std::get_if<unidb::sql::SqlParseError>(&parseResult)) {
    const auto code = error->code == unidb::sql::SqlParseErrorCode::SyntaxError
                          ? QueryExecutionErrorCode::SyntaxError
                          : QueryExecutionErrorCode::UnsupportedStatement;
    return QueryExecutionError{code, error->message};
  }

  try {
    return executeStatement(std::get<SqlStatement>(parseResult));
  } catch (const std::exception &error) {
    return executionError(error.what());
  } catch (...) {
    return executionError("Unknown query execution failure");
  }
}

QueryExecutionOutcome
ExecuteQueryAction::executeStatement(const SqlStatement &statement) const {
  if (const auto *create = std::get_if<CreateTableStatement>(&statement)) {
    domain::validateCreateTable(*create);
    rdbServicePort.createTable(*create);
    return commandResult("Table created");
  }

  if (const auto *insert = std::get_if<InsertStatement>(&statement)) {
    const auto schema = rdbServicePort.getTableDefinition(insert->tableName);
    const auto orderedRows = domain::prepareInsertRows(*insert, schema);
    const auto types = domain::schemaTypes(schema);
    std::vector<std::vector<char>> encodedRows;
    encodedRows.reserve(orderedRows.size());
    for (const auto &values : orderedRows) {
      encodedRows.push_back(SqlTupleCodec::encode(types, values));
    }

    std::uint32_t insertedRows = 0;
    try {
      for (const auto &bytes : encodedRows) {
        try {
          rdbServicePort.insertRow(insert->tableName, bytes);
          ++insertedRows;
        } catch (const RowInsertedButPostInsertUpdateFailed &) {
          ++insertedRows;
          throw;
        }
      }
    } catch (const std::exception &error) {
      return executionError("INSERT failed after " +
                            std::to_string(insertedRows) +
                            " row(s) were inserted: " + error.what());
    }
    return commandResult("Rows inserted", insertedRows);
  }

  if (const auto *unionAll = std::get_if<UnionAllStatement>(&statement)) {
    std::vector<QueryExecutionResult> branchResults;
    branchResults.reserve(unionAll->selects.size());
    for (const auto &select : unionAll->selects) {
      const auto branchOutcome = executeStatement(SqlStatement{select});
      if (const auto *error =
              std::get_if<QueryExecutionError>(&branchOutcome)) {
        return *error;
      }
      branchResults.push_back(std::get<QueryExecutionResult>(branchOutcome));
    }
    return domain::combineUnionResults(branchResults, false);
  }

  if (const auto *unionDistinct = std::get_if<UnionStatement>(&statement)) {
    std::vector<QueryExecutionResult> branchResults;
    branchResults.reserve(unionDistinct->selects.size());
    for (const auto &select : unionDistinct->selects) {
      const auto branchOutcome = executeStatement(SqlStatement{select});
      if (const auto *error =
              std::get_if<QueryExecutionError>(&branchOutcome)) {
        return *error;
      }
      branchResults.push_back(std::get<QueryExecutionResult>(branchOutcome));
    }
    return domain::combineUnionResults(branchResults, true);
  }

  if (const auto *select = std::get_if<SelectStatement>(&statement)) {
    if (select->crossJoin.has_value()) {
      if (!select->innerJoins.empty()) {
        throw std::invalid_argument(
            "Mixing CROSS JOIN and INNER JOIN is not supported");
      }
      const auto leftSchema =
          rdbServicePort.getTableDefinition(select->tableName);
      const auto rightSchema =
          rdbServicePort.getTableDefinition(select->crossJoin->tableName);
      domain::schemaTypes(leftSchema);
      domain::schemaTypes(rightSchema);
      return executeCrossJoin(rdbServicePort, *select, leftSchema, rightSchema);
    }
    if (!select->innerJoins.empty()) {
      std::vector<std::vector<ColumnDefinition>> schemas;
      schemas.push_back(rdbServicePort.getTableDefinition(select->tableName));
      for (const auto &join : select->innerJoins)
        schemas.push_back(rdbServicePort.getTableDefinition(join.tableName));
      for (const auto &schema : schemas)
        domain::schemaTypes(schema);
      if (select->innerJoins.size() == 1) {
        domain::validateInnerJoinColumns(*select, schemas[0], schemas[1]);
        return executeInnerJoin(rdbServicePort, *select, schemas[0],
                                schemas[1]);
      }
      return executeMultipleInnerJoins(rdbServicePort, *select, schemas);
    }
    const auto schema = rdbServicePort.getTableDefinition(select->tableName);
    const auto plan = domain::buildSelectPlan(*select, schema);
    const auto &types = plan.types;
    const auto encodedRows = rdbServicePort.selectAllRows(select->tableName);
    std::vector<std::vector<SqlValue>> matchedRows;
    matchedRows.reserve(encodedRows.size());
    for (const auto &bytes : encodedRows) {
      auto decoded = SqlTupleCodec::decode(types, bytes);
      if (plan.compiledWhere.has_value() &&
          !domain::matchesWhereExpression(decoded, *plan.compiledWhere)) {
        continue;
      }
      matchedRows.push_back(std::move(decoded));
    }

    return domain::processSelectRows(*select, plan, std::move(matchedRows));
  }

  return QueryExecutionError{QueryExecutionErrorCode::UnsupportedStatement,
                             "Statement is outside the V1 SQL subset"};
}

} // namespace QueryExecutionService
