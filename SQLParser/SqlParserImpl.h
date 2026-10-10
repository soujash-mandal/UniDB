#pragma once

#include "../SQLParserContract/sqlParser.h"
#include "SqlLexer.h"

namespace unidb::sql {

class SqlParserImpl final : public SqlParser {
public:
  SqlParseResult parse(std::string_view sql) const override;

private:
  implementation::SqlLexer lexer_;
};

} // namespace unidb::sql
