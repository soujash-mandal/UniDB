#include "ConnectionProtocol/SqlConnectionServer.h"
#include "DatabaseStartup.h"
#include "cli/SqlShell.h"
#include "container/container.h"

#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <vector>

#include <algorithm>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#if defined(__APPLE__)
#include <libproc.h>
#include <sys/proc_info.h>
#endif

namespace {
constexpr std::string_view kDefaultDatabasePath = "database.db";
constexpr unsigned int kDefaultPort = 7432;

// Resolve aliases before deriving lifecycle files so symlinked paths share
// state.
std::string lifecycleKey(std::string_view databasePath) {
  std::error_code error;
  const auto canonical = std::filesystem::weakly_canonical(
      std::filesystem::path(std::string(databasePath)), error);
  return error ? std::string(databasePath) : canonical.string();
}

std::string pidFilePath(std::string_view databasePath) {
  return lifecycleKey(databasePath) + ".pid";
}

std::string logFilePath(std::string_view databasePath) {
  return lifecycleKey(databasePath) + ".log";
}

// Serialize lifecycle operations for a database path across processes.
class LifecycleLock {
public:
  explicit LifecycleLock(std::string_view databasePath) {
    const std::string lockPath = lifecycleKey(databasePath) + ".lifecycle.lock";
    descriptor_ = ::open(lockPath.c_str(), O_CREAT | O_RDWR | O_CLOEXEC, 0644);
    if (descriptor_ >= 0 && ::flock(descriptor_, LOCK_EX) != 0) {
      ::close(descriptor_);
      descriptor_ = -1;
    }
  }

  ~LifecycleLock() {
    if (descriptor_ >= 0) {
      ::flock(descriptor_, LOCK_UN);
      ::close(descriptor_);
    }
  }

  LifecycleLock(const LifecycleLock &) = delete;
  LifecycleLock &operator=(const LifecycleLock &) = delete;

  bool acquired() const { return descriptor_ >= 0; }

private:
  int descriptor_ = -1;
};

// Keep only one process at a time opening a database with its own buffer pool.
class DatabaseOwnershipLock {
public:
  explicit DatabaseOwnershipLock(std::string_view databasePath) {
    // Existing hard links share the same device/inode lock key. For a database
    // that has not been created yet, use its canonical path without creating
    // the database file before DatabaseStartup can initialize it.
    struct stat databaseInfo{};
    std::string lockKey;
    const std::string path(databasePath);
    if (::stat(path.c_str(), &databaseInfo) == 0) {
      lockKey =
          std::to_string(static_cast<std::uintmax_t>(databaseInfo.st_dev)) +
          "-" +
          std::to_string(static_cast<std::uintmax_t>(databaseInfo.st_ino));
    } else if (errno == ENOENT) {
      // Use a deterministic hash so the key is stable across processes.
      std::uint64_t pathHash = 14695981039346656037ULL;
      for (const unsigned char character : lifecycleKey(databasePath)) {
        pathHash ^= character;
        pathHash *= 1099511628211ULL;
      }
      lockKey = std::to_string(pathHash);
    } else {
      return;
    }

    // Use a fixed system-wide location; TMPDIR can differ between processes.
    const std::string lockPath = "/tmp/unidb-ownership-" + lockKey + ".lock";
    descriptor_ = ::open(lockPath.c_str(), O_CREAT | O_RDWR | O_CLOEXEC, 0600);
    if (descriptor_ >= 0 && ::flock(descriptor_, LOCK_EX | LOCK_NB) != 0) {
      ::close(descriptor_);
      descriptor_ = -1;
    }
  }

  ~DatabaseOwnershipLock() {
    if (descriptor_ >= 0) {
      ::flock(descriptor_, LOCK_UN);
      ::close(descriptor_);
    }
  }

  DatabaseOwnershipLock(const DatabaseOwnershipLock &) = delete;
  DatabaseOwnershipLock &operator=(const DatabaseOwnershipLock &) = delete;

  bool acquired() const { return descriptor_ >= 0; }

private:
  int descriptor_ = -1;
};

// Return a process-start token so a reused PID cannot identify another process.
std::string processIdentity(pid_t pid) {
#if defined(__APPLE__)
  proc_bsdinfo info{};
  const int size = proc_pidinfo(pid, PROC_PIDTBSDINFO, 0, &info, sizeof(info));
  if (size != sizeof(info))
    return {};
  return std::to_string(info.pbi_start_tvsec) + ":" +
         std::to_string(info.pbi_start_tvusec);
#elif defined(__linux__)
  std::ifstream statFile("/proc/" + std::to_string(pid) + "/stat");
  std::string statLine;
  if (!std::getline(statFile, statLine))
    return {};
  const auto commandEnd = statLine.rfind(')');
  if (commandEnd == std::string::npos || commandEnd + 2 >= statLine.size())
    return {};
  std::istringstream fields(statLine.substr(commandEnd + 2));
  std::string field;
  // After the command name, the first token is field 3; starttime is field 22.
  for (int fieldNumber = 3; fieldNumber <= 22; ++fieldNumber) {
    if (!(fields >> field))
      return {};
    if (fieldNumber == 22)
      return field;
  }
  return {};
#else
  (void)pid;
  return {};
#endif
}

bool sendAll(int socket, const char *data, std::size_t size) {
  while (size > 0) {
    const auto sent = ::send(socket, data, size, 0);
    if (sent < 0) {
      if (errno == EINTR)
        continue;
      return false;
    }
    if (sent == 0)
      return false;
    data += sent;
    size -= static_cast<std::size_t>(sent);
  }
  return true;
}

bool isPortReachable(unsigned int port) {
  const int probe = ::socket(AF_INET, SOCK_STREAM, 0);
  if (probe < 0)
    return false;
  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_port = htons(static_cast<std::uint16_t>(port));
  address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  const bool reachable =
      ::connect(probe, reinterpret_cast<sockaddr *>(&address),
                sizeof(address)) == 0;
  ::close(probe);
  return reachable;
}

int connectToServer(unsigned int port) {
  const int socket = ::socket(AF_INET, SOCK_STREAM, 0);
  if (socket < 0) {
    std::cerr << "UniDB could not create a client socket: "
              << std::strerror(errno) << '\n';
    return -1;
  }

  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_port = htons(static_cast<std::uint16_t>(port));
  address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  if (::connect(socket, reinterpret_cast<sockaddr *>(&address),
                sizeof(address)) != 0) {
    std::cerr << "Could not connect to UniDB at 127.0.0.1:" << port << ": "
              << std::strerror(errno) << '\n';
    ::close(socket);
    return -1;
  }
  return socket;
}

int startServer(const char *executable, std::string_view databasePath,
                unsigned int port) {
#if !defined(__APPLE__) && !defined(__linux__)
  (void)executable;
  (void)databasePath;
  (void)port;
  std::cerr << "The 'start' command currently supports macOS and Linux only.\n";
  return 1;
#else
  LifecycleLock lifecycleLock(databasePath);
  if (!lifecycleLock.acquired()) {
    std::cerr << "Could not acquire UniDB lifecycle lock for database: "
              << databasePath << '\n';
    return 1;
  }
  const std::string pidPath = pidFilePath(databasePath);
  const std::string logPath = logFilePath(databasePath);

  {
    std::ifstream pidFile(pidPath);
    long existingPid = 0;
    unsigned int recordedPort = 0;
    std::string recordedIdentity;
    if (pidFile >> existingPid >> recordedPort) {
      const bool validPid =
          existingPid > 1 &&
          existingPid <= static_cast<long>(std::numeric_limits<pid_t>::max());
      const auto existingProcess = static_cast<pid_t>(existingPid);
      const bool processAlive =
          validPid && (::kill(existingProcess, 0) == 0 || errno == EPERM);
      if (processAlive) {
        if (!(pidFile >> recordedIdentity) || recordedIdentity.empty()) {
          std::cerr << "PID file refers to a live process but has no process "
                       "identity; refusing to remove it automatically.\n";
          return 1;
        }
        const std::string currentIdentity = processIdentity(existingProcess);
        if (currentIdentity.empty()) {
          std::cerr << "Could not verify the live process identity in "
                    << pidPath << "; refusing to remove the PID file.\n";
          return 1;
        }
        if (currentIdentity == recordedIdentity) {
          if (recordedPort == port && isPortReachable(recordedPort)) {
            std::cerr << "UniDB may already be running (PID " << existingPid
                      << "). Use 'unidb stop' before starting it again.\n";
          } else {
            std::cerr
                << "A UniDB process (PID " << existingPid
                << ") is still alive; preserving its PID record rather "
                   "than risking a second server for the same database.\n";
          }
          return 1;
        }
      }
      // The PID is dead or its start token proves it has been reused.
      std::remove(pidPath.c_str());
    } else {
      std::remove(pidPath.c_str());
    }
  }

  // Fail before forking if another listener already owns this port.
  const int existingListener = ::socket(AF_INET, SOCK_STREAM, 0);
  if (existingListener >= 0) {
    sockaddr_in existingAddress{};
    existingAddress.sin_family = AF_INET;
    existingAddress.sin_port = htons(static_cast<std::uint16_t>(port));
    existingAddress.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    const bool portInUse =
        ::connect(existingListener,
                  reinterpret_cast<sockaddr *>(&existingAddress),
                  sizeof(existingAddress)) == 0;
    ::close(existingListener);
    if (portInUse) {
      std::cerr << "Port " << port << " is already in use on 127.0.0.1.\n";
      return 1;
    }
  }

  // Keep the child blocked until its PID record is written. If this launcher
  // exits before releasing it, EOF makes the child exit before opening the DB.
  int startupPipe[2] = {-1, -1};
  if (::pipe(startupPipe) != 0) {
    std::cerr << "Could not create UniDB startup handshake: "
              << std::strerror(errno) << '\n';
    return 1;
  }

  const pid_t child = ::fork();
  if (child < 0) {
    const int forkError = errno;
    ::close(startupPipe[0]);
    ::close(startupPipe[1]);
    std::cerr << "Could not start UniDB: " << std::strerror(forkError) << '\n';
    return 1;
  }
  if (child == 0) {
    ::close(startupPipe[1]);
    char startupSignal = 0;
    ssize_t bytesRead = -1;
    do {
      bytesRead = ::read(startupPipe[0], &startupSignal, 1);
    } while (bytesRead < 0 && errno == EINTR);
    ::close(startupPipe[0]);
    if (bytesRead != 1 || startupSignal != 'G')
      _exit(125);

    if (::setsid() < 0)
      _exit(126);
    const int logFd =
        ::open(logPath.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (logFd < 0)
      _exit(126);
    ::dup2(logFd, STDOUT_FILENO);
    ::dup2(logFd, STDERR_FILENO);
    if (logFd > STDERR_FILENO)
      ::close(logFd);

    const std::string databaseArg(databasePath);
    const std::string portArg = std::to_string(port);
    char *const args[] = {const_cast<char *>(executable),
                          const_cast<char *>("--server"),
                          const_cast<char *>("--db-path"),
                          const_cast<char *>(databaseArg.c_str()),
                          const_cast<char *>("--port"),
                          const_cast<char *>(portArg.c_str()),
                          nullptr};
    ::execvp(executable, args);
    _exit(127);
  }

  ::close(startupPipe[0]);
  // Prevent a broken handshake pipe from terminating the launcher before
  // cleanup.
  std::signal(SIGPIPE, SIG_IGN);

  {
    const std::string identity = processIdentity(child);
    std::ofstream pidFile(pidPath, std::ios::trunc);
    bool pidFileWritten = !identity.empty() && static_cast<bool>(pidFile);
    if (pidFileWritten) {
      pidFile << child << '\n' << port << '\n' << identity << '\n';
      pidFile.flush();
      pidFileWritten = static_cast<bool>(pidFile);
      pidFile.close();
      pidFileWritten = pidFileWritten && !pidFile.fail();
    }
    if (!pidFileWritten) {
      std::cerr << "Could not write PID file with process identity: " << pidPath
                << '\n';
      ::close(startupPipe[1]);
      ::kill(child, SIGTERM);
      ::waitpid(child, nullptr, 0);
      std::remove(pidPath.c_str());
      return 1;
    }
  }

  // The complete PID record exists before the child can initialize storage.
  const char startupSignal = 'G';
  ssize_t bytesWritten = -1;
  do {
    bytesWritten = ::write(startupPipe[1], &startupSignal, 1);
  } while (bytesWritten < 0 && errno == EINTR);
  ::close(startupPipe[1]);
  if (bytesWritten != 1) {
    std::cerr << "Could not release UniDB startup handshake.\n";
    ::kill(child, SIGTERM);
    ::waitpid(child, nullptr, 0);
    std::remove(pidPath.c_str());
    return 1;
  }

  // Wait for the listener to become reachable, or report startup failure.
  for (int attempt = 0; attempt < 50; ++attempt) {
    int status = 0;
    if (::waitpid(child, &status, WNOHANG) == child) {
      std::remove(pidPath.c_str());
      std::cerr << "UniDB failed to start. Check the log: " << logPath << '\n';
      return 1;
    }
    const int probe = ::socket(AF_INET, SOCK_STREAM, 0);
    if (probe >= 0) {
      sockaddr_in address{};
      address.sin_family = AF_INET;
      address.sin_port = htons(static_cast<std::uint16_t>(port));
      address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
      const bool ready =
          ::connect(probe, reinterpret_cast<sockaddr *>(&address),
                    sizeof(address)) == 0;
      ::close(probe);
      int childStatus = 0;
      const pid_t childState = ::waitpid(child, &childStatus, WNOHANG);
      if (ready && childState == 0) {
        std::cout << "UniDB started (PID " << child << ") on 127.0.0.1:" << port
                  << "\nLog: " << logPath << '\n';
        return 0;
      }
      if (childState == child) {
        std::remove(pidPath.c_str());
        std::cerr << "UniDB failed to start. Check the log: " << logPath
                  << '\n';
        return 1;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  std::cerr << "Timed out waiting for UniDB to listen. Check the log: "
            << logPath << '\n';
  ::kill(child, SIGTERM);
  ::waitpid(child, nullptr, 0);
  std::remove(pidPath.c_str());
  return 1;
#endif
}

int stopServer(std::string_view databasePath, unsigned int port) {
  // Serialize stop with start so stale-file cleanup cannot remove a replacement
  // server's PID record.
  LifecycleLock lifecycleLock(databasePath);
  if (!lifecycleLock.acquired()) {
    std::cerr << "Could not acquire UniDB lifecycle lock for database: "
              << databasePath << '\n';
    return 1;
  }

  const std::string pidPath = pidFilePath(databasePath);
  std::ifstream pidFile(pidPath);
  long rawPid = 0;
  unsigned int recordedPort = 0;
  std::string recordedIdentity;
  if (!(pidFile >> rawPid >> recordedPort >> recordedIdentity) || rawPid <= 1 ||
      recordedPort == 0 || recordedPort > 65535) {
    std::cerr << "UniDB PID file not found or invalid: " << pidPath << '\n';
    return 1;
  }
  if (rawPid > static_cast<long>(std::numeric_limits<pid_t>::max())) {
    std::cerr << "UniDB PID file contains an out-of-range PID: " << pidPath
              << '\n';
    return 1;
  }
  const auto pid = static_cast<pid_t>(rawPid);
  if (::kill(pid, 0) != 0 && errno == ESRCH) {
    std::remove(pidPath.c_str());
    std::cerr << "Removed stale PID file; UniDB is not running.\n";
    return 1;
  }
  const std::string currentIdentity = processIdentity(pid);
  if (currentIdentity.empty()) {
    // An unavailable identity does not prove that the PID is stale. Preserve
    // the record unless the kernel confirms that the process no longer exists.
    if (::kill(pid, 0) != 0 && errno == ESRCH) {
      std::remove(pidPath.c_str());
      std::cerr << "Removed stale PID file; UniDB is not running.\n";
      return 1;
    }
    std::cerr << "Could not verify the process identity in " << pidPath
              << "; refusing to signal or remove the PID file.\n";
    return 1;
  }
  if (currentIdentity != recordedIdentity) {
    std::remove(pidPath.c_str());
    std::cerr
        << "PID no longer identifies the UniDB process; removed stale PID "
           "file without sending a signal.\n";
    return 1;
  }
  if (recordedPort != port || !isPortReachable(recordedPort)) {
    std::cerr << "PID file does not match a reachable UniDB listener on port "
              << recordedPort << "; refusing to signal PID " << rawPid << ".\n";
    return 1;
  }
  if (::kill(pid, SIGTERM) != 0) {
    std::cerr << "Could not request UniDB shutdown: " << std::strerror(errno)
              << '\n';
    return 1;
  }

  for (int attempt = 0; attempt < 100; ++attempt) {
    if (::kill(pid, 0) != 0 && errno == ESRCH) {
      std::remove(pidPath.c_str());
      std::cout << "UniDB stopped; graceful shutdown completed.\n";
      return 0;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  std::cerr << "UniDB did not exit within 10 seconds. Check "
            << logFilePath(databasePath) << '\n';
  return 1;
}

int connectInteractive(unsigned int port) {
  // Ignore SIGPIPE so a disconnected server is handled through send errors.
  std::signal(SIGPIPE, SIG_IGN);
  const int socket = connectToServer(port);
  if (socket < 0)
    return 1;

  const int socketFlags = ::fcntl(socket, F_GETFL, 0);
  if (socketFlags < 0 ||
      ::fcntl(socket, F_SETFL, socketFlags | O_NONBLOCK) < 0) {
    std::cerr << "Could not make the UniDB client socket nonblocking: "
              << std::strerror(errno) << '\n';
    ::close(socket);
    return 1;
  }

  std::cout << "Connected to UniDB at 127.0.0.1:" << port
            << ". Enter one SQL statement per line; Ctrl+D disconnects.\n";

  constexpr std::size_t kMaxPendingInput = 64 * 1024;
  pollfd descriptors[2] = {};
  char buffer[4096];
  std::string pendingInput;
  std::size_t pendingOffset = 0;
  bool stdinOpen = true;
  bool sentInput = false;
  bool inputEndsWithNewline = false;
  bool writeShutdown = false;

  while (true) {
    const std::size_t pendingBytes = pendingInput.size() - pendingOffset;
    descriptors[0] = {
        stdinOpen && pendingBytes < kMaxPendingInput ? STDIN_FILENO : -1,
        POLLIN, 0};
    descriptors[1] = {
        socket, static_cast<short>(POLLIN | (pendingBytes > 0 ? POLLOUT : 0)),
        0};

    const int ready = ::poll(descriptors, 2, -1);
    if (ready < 0) {
      if (errno == EINTR)
        continue;
      std::cerr << "Client poll failed: " << std::strerror(errno) << '\n';
      ::close(socket);
      return 1;
    }

    if (stdinOpen && (descriptors[0].revents & (POLLIN | POLLHUP))) {
      const std::size_t available = kMaxPendingInput - pendingBytes;
      const auto requested = std::min(sizeof(buffer), available);
      const auto count = ::read(STDIN_FILENO, buffer, requested);
      if (count == 0) {
        stdinOpen = false;
        if (sentInput && !inputEndsWithNewline) {
          pendingInput.push_back('\n');
          inputEndsWithNewline = true;
        }
      } else if (count < 0) {
        if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
          std::cerr << "Failed to read SQL input: " << std::strerror(errno)
                    << '\n';
          ::close(socket);
          return 1;
        }
      } else {
        pendingInput.append(buffer, static_cast<std::size_t>(count));
        sentInput = true;
        inputEndsWithNewline = buffer[count - 1] == '\n';
      }
    }

    if (descriptors[1].revents & POLLOUT) {
      const auto pendingBytesNow = pendingInput.size() - pendingOffset;
      if (pendingBytesNow > 0) {
        const auto count = ::send(socket, pendingInput.data() + pendingOffset,
                                  pendingBytesNow, 0);
        if (count > 0) {
          pendingOffset += static_cast<std::size_t>(count);
          if (pendingOffset == pendingInput.size()) {
            pendingInput.clear();
            pendingOffset = 0;
          } else if (pendingOffset >= kMaxPendingInput / 2) {
            pendingInput.erase(0, pendingOffset);
            pendingOffset = 0;
          }
        } else if (count < 0 && errno != EINTR && errno != EAGAIN &&
                   errno != EWOULDBLOCK) {
          std::cerr << "Failed to send SQL to UniDB: " << std::strerror(errno)
                    << '\n';
          ::close(socket);
          return 1;
        }
      }
    }

    // Half-close only after every queued byte (including the final newline)
    // has been sent, so EOF cannot discard pending SQL.
    if (!stdinOpen && pendingInput.empty() && !writeShutdown) {
      ::shutdown(socket, SHUT_WR);
      writeShutdown = true;
    }

    if (descriptors[1].revents & (POLLIN | POLLHUP | POLLERR | POLLNVAL)) {
      const auto count = ::recv(socket, buffer, sizeof(buffer), 0);
      if (count > 0) {
        std::cout.write(buffer, count);
        std::cout.flush();
      } else if (count == 0) {
        break;
      } else if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
        std::cerr << "Failed to receive response from UniDB: "
                  << std::strerror(errno) << '\n';
        ::close(socket);
        return 1;
      }
    }
  }

  ::close(socket);
  return 0;
}
} // namespace

int main(int argc, char *argv[]) {
  std::string_view command;
  int firstOption = 1;
  if (argc > 1) {
    const std::string_view first(argv[1]);
    if (first == "start" || first == "connect" || first == "stop") {
      command = first;
      firstOption = 2;
    }
  }

  bool serverMode = false;
  std::string_view databasePath = kDefaultDatabasePath;
  unsigned int port = kDefaultPort;

  for (int index = firstOption; index < argc; ++index) {
    const std::string_view argument(argv[index]);
    if (argument == "--server") {
      serverMode = true;
    } else if (argument == "--port") {
      if (index + 1 >= argc) {
        std::cerr << "Usage: unidb [start|connect|stop] [--db-path PATH] "
                     "[--port PORT]\n";
        return 2;
      }
      try {
        const auto parsed = std::stoul(argv[++index]);
        if (parsed == 0 || parsed > 65535)
          throw std::out_of_range("port");
        port = static_cast<unsigned int>(parsed);
      } catch (const std::exception &) {
        std::cerr << "Invalid TCP port. Expected a number from 1 to 65535.\n";
        return 2;
      }
    } else if (argument == "--db-path") {
      if (index + 1 >= argc) {
        std::cerr << "Usage: unidb [start|connect|stop] [--db-path PATH] "
                     "[--port PORT]\n";
        return 2;
      }
      databasePath = argv[++index];
    } else if (argument == "--help") {
      std::cout
          << "Usage: unidb [start|connect|stop] [--server] [--db-path PATH] "
             "[--port PORT]\n"
             "  start          Start a background server and write a PID/log "
             "file.\n"
             "  connect        Open an interactive SQL connection to the "
             "server.\n"
             "  stop           Request graceful shutdown and wait for exit.\n"
             "  --server       Run the TCP server in the foreground (legacy "
             "mode).\n"
             "  --db-path PATH Database file path (default: database.db).\n"
             "  --port PORT    TCP port (default: 7432; loopback only).\n";
      return 0;
    } else {
      std::cerr << "Unknown argument: " << argument << '\n'
                << "Use --help for usage.\n";
      return 2;
    }
  }

  if (command == "start")
    return startServer(argv[0], databasePath, port);
  if (command == "connect")
    return connectInteractive(port);
  if (command == "stop")
    return stopServer(databasePath, port);

  DatabaseOwnershipLock databaseOwnershipLock(databasePath);
  if (!databaseOwnershipLock.acquired()) {
    std::cerr << "Another UniDB process already owns this database: "
              << databasePath << '\n'
              << "Stop the other process before opening this database again.\n";
    return 1;
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
