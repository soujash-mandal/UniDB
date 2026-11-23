#pragma once

#include "../../type/type.h"

namespace QueryExecutionService::domain {

// Compatibility aliases keep the temporal semantics API stable while the
// value representations themselves belong to the shared SQL contract.
using SqlDate = unidb::sql::SqlDate;
using SqlTime = unidb::sql::SqlTime;
using SqlTimeWithTimeZone = unidb::sql::SqlTimeWithTimeZone;
using SqlTimestamp = unidb::sql::SqlTimestamp;
using SqlTimestampWithTimeZone = unidb::sql::SqlTimestampWithTimeZone;
using SqlInterval = unidb::sql::SqlInterval;

} // namespace QueryExecutionService::domain
