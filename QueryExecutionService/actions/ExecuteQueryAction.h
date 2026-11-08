#pragma once

#include "../domain/contract/sqlParser.h"
#include "../type.h"
#include "../port/RdbServicePort.h"

#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace QueryExecutionService {

class ExecuteQueryAction {
public:
  ExecuteQueryAction(RdbServicePort &rdbServicePort,
                     const unidb::sql::SqlParser &sqlParser);

  QueryExecutionOutcome execute(std::string_view sql) const;

private:
  QueryExecutionOutcome
  executeStatement(const unidb::sql::SqlStatement &statement) const;

  RdbServicePort &rdbServicePort;
  const unidb::sql::SqlParser &sqlParser;
};

} // namespace QueryExecutionService
