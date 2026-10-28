#pragma once

#include <cstdint>

class Container;

namespace ConnectionProtocol {

// Local-only newline-delimited SQL server. Each complete request is executed
// through QueryExecutionService and answered with one JSON line.
class SqlConnectionServer {
public:
  explicit SqlConnectionServer(Container &container, std::uint16_t port = 7432);
  int run();

private:
  Container &container;
  std::uint16_t port;
};

} // namespace ConnectionProtocol
