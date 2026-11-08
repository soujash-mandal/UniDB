#include "SqlTypeRegistry.h"

#include <cctype>
#include <stdexcept>
#include <string>

namespace QueryExecutionService::domain {
namespace {

using unidb::sql::SqlTypeDescriptor;
using unidb::sql::SqlTypeId;
using unidb::sql::SqlTypeModifier;

std::string normalizeTypeName(std::string_view name) {
  std::string normalized;
  bool pendingSpace = false;

  for (const unsigned char character : name) {
    if (std::isspace(character)) {
      pendingSpace = !normalized.empty();
      continue;
    }

    if (pendingSpace) {
      normalized.push_back(' ');
      pendingSpace = false;
    }
    normalized.push_back(static_cast<char>(std::tolower(character)));
  }

  return normalized;
}

SqlTypeId resolveTypeId(const std::string &name) {
  if (name == "smallint" || name == "int2") {
    return SqlTypeId::SmallInt;
  }
  if (name == "integer" || name == "int" || name == "int4") {
    return SqlTypeId::Integer;
  }
  if (name == "bigint" || name == "int8") {
    return SqlTypeId::BigInt;
  }
  if (name == "real" || name == "float4") {
    return SqlTypeId::Real;
  }
  if (name == "double precision" || name == "float8") {
    return SqlTypeId::DoublePrecision;
  }
  if (name == "numeric" || name == "decimal") {
    return SqlTypeId::Numeric;
  }
  if (name == "boolean" || name == "bool") {
    return SqlTypeId::Boolean;
  }
  if (name == "character" || name == "char" || name == "bpchar") {
    return SqlTypeId::Char;
  }
  if (name == "character varying" || name == "varchar") {
    return SqlTypeId::Varchar;
  }
  if (name == "text") {
    return SqlTypeId::Text;
  }
  if (name == "date") {
    return SqlTypeId::Date;
  }
  if (name == "time" || name == "time without time zone") {
    return SqlTypeId::Time;
  }
  if (name == "time with time zone" || name == "timetz") {
    return SqlTypeId::TimeWithTimeZone;
  }
  if (name == "timestamp" || name == "timestamp without time zone") {
    return SqlTypeId::Timestamp;
  }
  if (name == "timestamp with time zone" || name == "timestamptz") {
    return SqlTypeId::TimestampWithTimeZone;
  }
  if (name == "interval") {
    return SqlTypeId::Interval;
  }
  if (name == "bytea") {
    return SqlTypeId::Bytea;
  }
  if (name == "uuid") {
    return SqlTypeId::Uuid;
  }
  if (name == "json") {
    return SqlTypeId::Json;
  }
  if (name == "jsonb") {
    return SqlTypeId::Jsonb;
  }

  throw std::invalid_argument("Unsupported SQL type: " + name);
}

void rejectUnexpectedModifiers(const SqlTypeModifier &modifier,
                               bool allowLength, bool allowNumeric,
                               bool allowTemporalPrecision) {
  if ((!allowLength && modifier.length.has_value()) ||
      (!allowNumeric &&
       (modifier.precision.has_value() || modifier.scale.has_value())) ||
      (!allowTemporalPrecision && modifier.temporalPrecision.has_value())) {
    throw std::invalid_argument("Modifiers are not valid for this SQL type");
  }
}

void validateModifiers(SqlTypeId type, SqlTypeModifier &modifier) {
  switch (type) {
  case SqlTypeId::Char:
    rejectUnexpectedModifiers(modifier, true, false, false);
    if (!modifier.length.has_value()) {
      modifier.length = 1;
    }
    if (*modifier.length == 0) {
      throw std::invalid_argument("CHAR length must be greater than zero");
    }
    return;
  case SqlTypeId::Varchar:
    rejectUnexpectedModifiers(modifier, true, false, false);
    if (modifier.length.has_value() && *modifier.length == 0) {
      throw std::invalid_argument("VARCHAR length must be greater than zero");
    }
    return;
  case SqlTypeId::Numeric:
    rejectUnexpectedModifiers(modifier, false, true, false);
    if (modifier.precision.has_value() &&
        (*modifier.precision == 0 || *modifier.precision > 1000)) {
      throw std::invalid_argument(
          "NUMERIC precision must be between 1 and 1000");
    }
    if (modifier.scale.has_value() && !modifier.precision.has_value()) {
      throw std::invalid_argument(
          "NUMERIC scale requires an explicit precision");
    }
    if (modifier.scale.has_value() &&
        (*modifier.scale < -1000 || *modifier.scale > 1000)) {
      throw std::invalid_argument(
          "NUMERIC scale must be between -1000 and 1000");
    }
    return;
  case SqlTypeId::Time:
  case SqlTypeId::TimeWithTimeZone:
  case SqlTypeId::Timestamp:
  case SqlTypeId::TimestampWithTimeZone:
    rejectUnexpectedModifiers(modifier, false, false, true);
    if (modifier.temporalPrecision.has_value() &&
        *modifier.temporalPrecision > 6) {
      throw std::invalid_argument("Temporal precision must be between 0 and 6");
    }
    return;
  default:
    rejectUnexpectedModifiers(modifier, false, false, false);
    return;
  }
}

} // namespace

unidb::sql::SqlTypeDescriptor
resolveSqlType(std::string_view name, unidb::sql::SqlTypeModifier modifier) {
  const auto normalized = normalizeTypeName(name);
  if (normalized.empty()) {
    throw std::invalid_argument("SQL type name cannot be empty");
  }

  const auto id = resolveTypeId(normalized);
  validateModifiers(id, modifier);
  return SqlTypeDescriptor{id, modifier};
}

std::string_view canonicalSqlTypeName(unidb::sql::SqlTypeId type) {
  switch (type) {
  case SqlTypeId::SmallInt:
    return "smallint";
  case SqlTypeId::Integer:
    return "integer";
  case SqlTypeId::BigInt:
    return "bigint";
  case SqlTypeId::Real:
    return "real";
  case SqlTypeId::DoublePrecision:
    return "double precision";
  case SqlTypeId::Numeric:
    return "numeric";
  case SqlTypeId::Boolean:
    return "boolean";
  case SqlTypeId::Char:
    return "character";
  case SqlTypeId::Varchar:
    return "character varying";
  case SqlTypeId::Text:
    return "text";
  case SqlTypeId::Date:
    return "date";
  case SqlTypeId::Time:
    return "time without time zone";
  case SqlTypeId::TimeWithTimeZone:
    return "time with time zone";
  case SqlTypeId::Timestamp:
    return "timestamp without time zone";
  case SqlTypeId::TimestampWithTimeZone:
    return "timestamp with time zone";
  case SqlTypeId::Interval:
    return "interval";
  case SqlTypeId::Bytea:
    return "bytea";
  case SqlTypeId::Uuid:
    return "uuid";
  case SqlTypeId::Json:
    return "json";
  case SqlTypeId::Jsonb:
    return "jsonb";
  }

  throw std::invalid_argument("Unknown SQL type identifier");
}

} // namespace QueryExecutionService::domain
