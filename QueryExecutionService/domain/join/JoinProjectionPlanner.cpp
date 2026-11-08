#include "JoinProjectionPlanner.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>

namespace QueryExecutionService::domain {
namespace {

using unidb::sql::ColumnDefinition;
using unidb::sql::SelectStatement;

std::size_t
countColumnName(const std::string &name,
                const std::vector<std::vector<ColumnDefinition>> &schemas) {
  std::size_t count = 0;
  for (const auto &schema : schemas) {
    count += static_cast<std::size_t>(
        std::count_if(schema.begin(), schema.end(),
                      [&](const auto &column) { return column.name == name; }));
  }
  return count;
}

} // namespace

JoinProjectionPlan buildJoinProjectionPlan(
    const SelectStatement &select,
    const std::vector<std::vector<ColumnDefinition>> &schemas,
    const std::vector<std::string> &qualifiers) {
  if (schemas.empty() || schemas.size() != qualifiers.size()) {
    throw std::invalid_argument("Invalid JOIN projection schema");
  }
  if (!select.selectAll && select.columnNames.empty()) {
    throw std::invalid_argument("SELECT requires at least one column");
  }
  if (!select.columnAliases.empty() &&
      select.columnAliases.size() != select.columnNames.size()) {
    throw std::invalid_argument("SELECT alias count does not match columns");
  }

  JoinProjectionPlan plan;
  if (select.selectAll) {
    for (std::size_t tableIndex = 0; tableIndex < schemas.size();
         ++tableIndex) {
      for (const auto &column : schemas[tableIndex]) {
        const bool duplicated = countColumnName(column.name, schemas) > 1;
        plan.indexes.push_back(plan.indexes.size());
        plan.columns.push_back({duplicated
                                    ? qualifiers[tableIndex] + "." + column.name
                                    : column.name,
                                column.type});
      }
    }
  } else {
    for (std::size_t columnIndex = 0; columnIndex < select.columnNames.size();
         ++columnIndex) {
      const auto &requestedName = select.columnNames[columnIndex];
      const auto separator = requestedName.find('.');
      std::size_t tableIndex = 0;
      const ColumnDefinition *resolvedColumn = nullptr;
      std::size_t columnIndexInTable = 0;

      if (separator != std::string::npos) {
        if (requestedName.find('.', separator + 1) != std::string::npos) {
          throw std::invalid_argument("Invalid qualified SELECT column: " +
                                      requestedName);
        }
        const auto qualifier = requestedName.substr(0, separator);
        const auto columnName = requestedName.substr(separator + 1);
        const auto table =
            std::find(qualifiers.begin(), qualifiers.end(), qualifier);
        if (table == qualifiers.end()) {
          throw std::invalid_argument("Unknown SELECT table: " + qualifier);
        }
        tableIndex =
            static_cast<std::size_t>(std::distance(qualifiers.begin(), table));
        const auto &schema = schemas[tableIndex];
        const auto column =
            std::find_if(schema.begin(), schema.end(), [&](const auto &entry) {
              return entry.name == columnName;
            });
        if (column == schema.end()) {
          throw std::invalid_argument("Unknown SELECT column: " +
                                      requestedName);
        }
        columnIndexInTable =
            static_cast<std::size_t>(std::distance(schema.begin(), column));
        resolvedColumn = &*column;
      } else {
        for (std::size_t candidateTable = 0; candidateTable < schemas.size();
             ++candidateTable) {
          const auto &schema = schemas[candidateTable];
          const auto column = std::find_if(
              schema.begin(), schema.end(),
              [&](const auto &entry) { return entry.name == requestedName; });
          if (column == schema.end()) {
            continue;
          }
          if (resolvedColumn != nullptr) {
            throw std::invalid_argument("Ambiguous SELECT column: " +
                                        requestedName);
          }
          tableIndex = candidateTable;
          columnIndexInTable =
              static_cast<std::size_t>(std::distance(schema.begin(), column));
          resolvedColumn = &*column;
        }
        if (resolvedColumn == nullptr) {
          throw std::invalid_argument("Unknown SELECT column: " +
                                      requestedName);
        }
      }

      std::size_t offset = columnIndexInTable;
      for (std::size_t priorTable = 0; priorTable < tableIndex; ++priorTable) {
        offset += schemas[priorTable].size();
      }
      plan.indexes.push_back(offset);

      std::string outputName = resolvedColumn->name;
      if (countColumnName(resolvedColumn->name, schemas) > 1) {
        outputName = qualifiers[tableIndex] + "." + resolvedColumn->name;
      }
      if (columnIndex < select.columnAliases.size() &&
          select.columnAliases[columnIndex].has_value()) {
        outputName = *select.columnAliases[columnIndex];
      }
      plan.columns.push_back({std::move(outputName), resolvedColumn->type});
    }
  }

  std::unordered_set<std::string> uniqueOutputNames;
  for (const auto &column : plan.columns) {
    if (!uniqueOutputNames.insert(column.name).second) {
      throw std::invalid_argument("Duplicate SELECT output column name: " +
                                  column.name);
    }
  }
  return plan;
}

} // namespace QueryExecutionService::domain
