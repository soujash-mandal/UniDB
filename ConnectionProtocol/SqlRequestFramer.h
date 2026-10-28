#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace ConnectionProtocol {

// The initial local-client protocol accepts one UTF-8 SQL statement per line.
inline constexpr std::size_t MAX_REQUEST_BYTES = 64 * 1024;

// Buffers bytes from a socket and returns each complete newline-delimited SQL
// request. The caller retains the framer between reads for each connection.
// Throws std::length_error when a request exceeds MAX_REQUEST_BYTES and
// std::invalid_argument for an empty request.
class SqlRequestFramer {
public:
  std::vector<std::string> append(std::string_view bytes);
  bool hasPendingBytes() const { return !pending.empty(); }

private:
  std::string pending;
};

} // namespace ConnectionProtocol
