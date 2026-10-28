#include "DatabaseStartup.h"
#include "ConnectionProtocol/SqlConnectionServer.h"
#include "cli/SqlShell.h"
#include "container/container.h"

#include <cstdint>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>

int main(int argc, char *argv[]) {
  bool serverMode = false;
  std::string_view databasePath = "database.db";
  unsigned int port = 7432;

  for (int index = 1; index < argc; ++index) {
    const std::string_view argument(argv[index]);
    if (argument == "--server") {
      serverMode = true;
    } else if (argument == "--port") {
      if (index + 1 >= argc) {
        std::cerr << "Usage: unidb [--server] [--db-path PATH] [--port PORT]\n";
        return 2;
      }
      try {
        const auto parsed = std::stoul(argv[++index]);
        if (parsed == 0 || parsed > 65535) throw std::out_of_range("port");
        port = static_cast<unsigned int>(parsed);
      } catch (const std::exception &) {
        std::cerr << "Invalid TCP port. Expected a number from 1 to 65535.\n";
        return 2;
      }
    } else if (argument == "--db-path") {
      if (index + 1 >= argc) {
        std::cerr << "Usage: unidb [--server] [--db-path PATH] [--port PORT]\n";
        return 2;
      }
      databasePath = argv[++index];
    } else if (argument == "--help") {
      std::cout
          << "Usage: unidb [--server] [--db-path PATH] [--port PORT]\n"
             "  --server       Accept newline-delimited SQL over loopback TCP.\n"
             "  --db-path PATH Database file path (default: database.db).\n"
             "  --port PORT    TCP port (default: 7432; loopback only).\n";
      return 0;
    } else {
      std::cerr << "Unknown argument: " << argument << '\n'
                << "Use --help for usage.\n";
      return 2;
    }
  }

  try {
    Container container(std::string(databasePath), 2, EvictionPolicyType::FIFO);

    const bool initializedNewDatabase = DatabaseStartup::execute(container);
    if (initializedNewDatabase) {
      std::cout << "Initialized new UniDB database.\n";
    } else {
      std::cout << "Validated existing UniDB database.\n";
    }

    if (serverMode) {
      ConnectionProtocol::SqlConnectionServer server(
          container, static_cast<std::uint16_t>(port));
      return server.run();
    }

    SqlShell shell(container.executeQueryAction(), std::cin, std::cout,
                   std::cerr);
    shell.run();
    container.flushDirtyPages();
  } catch (const std::exception &error) {
    std::cerr << "UniDB failed: " << error.what() << '\n';
    return 1;
  }

  return 0;
}
