#include "ExecuteQueryAction.h"

#include "../domain/codec/SqlTupleCodec.h"
#include "../domain/join/JoinExecutor.h"
#include "../domain/join/JoinValidator.h"
#include "../domain/planning/SqlSelectPlanner.h"
#include "../domain/planning/SqlSelectResultProcessor.h"
#include "../domain/planning/SqlStatementRules.h"
#include "../domain/schema/SqlSchemaResolver.h"

#include <stdexcept>
#include <utility>

namespace QueryExecutionService {
namespace {

// JOIN execution remains here because these paths read rows through
// RdbServicePort; their independent planning and result rules belong in domain.
using unidb::sql::ColumnDefinition;
using unidb::sql::CreateTableStatement;
using unidb::sql::InsertStatement;
using unidb::sql::SelectStatement;
using unidb::sql::SqlStatement;
using unidb::sql::SqlValue;
using unidb::sql::UnionAllStatement;
using unidb::sql::UnionStatement;

QueryExecutionError executionError(std::string message) {
  return {QueryExecutionErrorCode::ExecutionError, std::move(message)};
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
