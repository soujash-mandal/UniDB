#pragma once

#include <cstdint>
#include <string>

class Container;

namespace ConnectionProtocol {

// Newline-delimited SQL server. The bind address defaults to loopback; set
// UNIDB_BIND_ADDRESS explicitly when a container must accept published ports.
class SqlConnectionServer {
public:
  explicit SqlConnectionServer(Container &container, std::uint16_t port = 7432,
                               std::string bindAddress = {});
  int run();

private:
  Container &container;
  std::uint16_t port;
  std::string bindAddress;
};

} // namespace ConnectionProtocol
