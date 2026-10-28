#include "DatabaseStartup.h"
#include "cli/SqlShell.h"
#include "container/container.h"

#include <chrono>
#include <csignal>
#include <exception>
#include <iostream>
#include <string_view>
#include <thread>

namespace {

// Signal handlers may only perform async-signal-safe operations. sig_atomic_t
// is used to communicate the shutdown request to the main thread.
volatile std::sig_atomic_t shutdownRequested = 0;

extern "C" void requestShutdown(int) { shutdownRequested = 1; }

bool installShutdownHandlers() {
  struct sigaction action {};
  action.sa_handler = requestShutdown;
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;

  return sigaction(SIGINT, &action, nullptr) == 0 &&
         sigaction(SIGTERM, &action, nullptr) == 0;
}

int runServer(Container &container) {
  if (!installShutdownHandlers()) {
    std::cerr << "UniDB failed: could not install shutdown signal handlers\n";
    return 1;
  }

  std::cout << "UniDB server lifecycle started. Waiting for SIGINT or SIGTERM.\n";
  while (shutdownRequested == 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  std::cout << "UniDB server shutting down; flushing dirty pages.\n";
  container.flushDirtyPages();
  std::cout << "UniDB server shutdown complete.\n";
  return 0;
}

} // namespace

int main(int argc, char *argv[]) {
  bool serverMode = false;
  std::string_view databasePath = "database.db";

  for (int index = 1; index < argc; ++index) {
    const std::string_view argument(argv[index]);
    if (argument == "--server") {
      serverMode = true;
    } else if (argument == "--db-path") {
      if (index + 1 >= argc) {
        std::cerr << "Usage: unidb [--server] [--db-path PATH]\n";
        return 2;
      }
      databasePath = argv[++index];
    } else if (argument == "--help") {
      std::cout << "Usage: unidb [--server] [--db-path PATH]\n"
                   "  --server       Run without the interactive SQL shell.\n"
                   "  --db-path PATH Database file path (default: database.db).\n";
      return 0;
    } else {
      std::cerr << "Unknown argument: " << argument << '\n'
                << "Use --help for usage.\n";
      return 2;
    }
  }

  try {
    Container container(std::string(databasePath), 2,
                        EvictionPolicyType::FIFO);

    const bool initializedNewDatabase = DatabaseStartup::execute(container);
    if (initializedNewDatabase) {
      std::cout << "Initialized new UniDB database.\n";
    } else {
      std::cout << "Validated existing UniDB database.\n";
    }

    if (serverMode) {
      return runServer(container);
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
