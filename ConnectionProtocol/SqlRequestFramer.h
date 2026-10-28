#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ConnectionProtocol {

// The initial local-client protocol accepts one UTF-8 SQL statement per line.
inline constexpr std::size_t MAX_REQUEST_BYTES = 64 * 1024;

struct SqlRequestFramingResult {
  std::vector<std::string> requests;
  std::optional<std::string> error;
};

// Buffers bytes from a socket and returns complete newline-delimited SQL
// requests. The caller retains the framer between reads for each connection.
// A framing error is returned alongside any valid requests extracted earlier
// from the same chunk. The 64 KiB limit excludes the CR in a CRLF terminator.
class SqlRequestFramer {
public:
  SqlRequestFramingResult append(std::string_view bytes);
  bool hasPendingBytes() const { return !pending.empty(); }

private:
  std::string pending;
};

} // namespace ConnectionProtocol
