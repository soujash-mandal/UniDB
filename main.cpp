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
#include <memory>
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
#include <pwd.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <openssl/err.h>
#include <openssl/ssl.h>

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

// Lock the database inode itself so every process with access to the same
// file (including processes running under different effective UIDs) contends
// on the same kernel lock. O_CREAT makes the absent-to-existing transition
// atomic: the lock is acquired before Container opens the database.
class DatabaseOwnershipLock {
public:
  explicit DatabaseOwnershipLock(std::string_view databasePath) {
    databasePath_ = std::string(databasePath);
    descriptor_ =
        ::open(databasePath_.c_str(), O_CREAT | O_RDWR | O_CLOEXEC, 0644);
    if (descriptor_ < 0) {
      failureReason_ = "could not open database file '" + databasePath_ +
                       "': " + std::strerror(errno);
      return;
    }

    struct stat info{};
    if (::fstat(descriptor_, &info) != 0) {
      failureReason_ = "could not inspect database file '" + databasePath_ +
                       "': " + std::strerror(errno);
      closeDescriptor();
      return;
    }
    if (!S_ISREG(info.st_mode)) {
      failureReason_ = "database path is not a regular file: " + databasePath_;
      closeDescriptor();
      return;
    }

    if (::flock(descriptor_, LOCK_EX | LOCK_NB) != 0) {
      const int lockError = errno;
      if (lockError == EWOULDBLOCK || lockError == EAGAIN) {
        contended_ = true;
        failureReason_ = "another UniDB process already owns database '" +
                         databasePath_ + "'";
      } else {
        failureReason_ = "could not lock database file '" + databasePath_ +
                         "': " + std::strerror(lockError);
      }
      closeDescriptor();
    }
  }

  ~DatabaseOwnershipLock() { closeDescriptor(); }

  DatabaseOwnershipLock(const DatabaseOwnershipLock &) = delete;
  DatabaseOwnershipLock &operator=(const DatabaseOwnershipLock &) = delete;

  bool acquired() const { return descriptor_ >= 0; }
  bool contended() const { return contended_; }
  const std::string &failureReason() const { return failureReason_; }

  // The database file is locked before startup and remains locked throughout
  // initialization and normal operation; there is no separate transition.
  bool acquireCreatedDatabaseLock() const { return acquired(); }

private:
  void closeDescriptor() {
    if (descriptor_ >= 0) {
      ::close(descriptor_);
      descriptor_ = -1;
    }
  }

  std::string databasePath_;
  std::string failureReason_;
  int descriptor_ = -1;
  bool contended_ = false;
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

std::string serverBindAddress() {
  const char *configuredAddress = std::getenv("UNIDB_BIND_ADDRESS");
  if (configuredAddress == nullptr || configuredAddress[0] == '\0')
    return "127.0.0.1";
  return configuredAddress;
}

std::string serverProbeAddress(std::string_view bindAddress) {
  return bindAddress == "0.0.0.0" ? "127.0.0.1" : std::string(bindAddress);
}

bool isPortReachable(unsigned int port, std::string_view configuredAddress) {
  const int probe = ::socket(AF_INET, SOCK_STREAM, 0);
  if (probe < 0)
    return false;
  const std::string probeAddress =
      configuredAddress.empty() || configuredAddress == "0.0.0.0"
          ? "127.0.0.1"
          : std::string(configuredAddress);
  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_port = htons(static_cast<std::uint16_t>(port));
  if (::inet_pton(AF_INET, probeAddress.c_str(), &address.sin_addr) != 1) {
    ::close(probe);
    return false;
  }
  const bool reachable =
      ::connect(probe, reinterpret_cast<sockaddr *>(&address),
                sizeof(address)) == 0;
  ::close(probe);
  return reachable;
}

bool isLoopbackIpv4(std::string_view address) {
  in_addr parsedAddress{};
  if (::inet_pton(AF_INET, std::string(address).c_str(), &parsedAddress) != 1)
    return false;
  return (ntohl(parsedAddress.s_addr) >> 24U) == 127U;
}

int connectToServer(unsigned int port, std::string_view destination) {
  const int socket = ::socket(AF_INET, SOCK_STREAM, 0);
  if (socket < 0) {
    std::cerr << "UniDB could not create a client socket: "
              << std::strerror(errno) << '\n';
    return -1;
  }

  const std::string destinationAddress(destination);
  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_port = htons(static_cast<std::uint16_t>(port));
  if (::inet_pton(AF_INET, destinationAddress.c_str(), &address.sin_addr) !=
      1) {
    std::cerr << "Invalid server IPv4 address: " << destination << '\n';
    ::close(socket);
    return -1;
  }
  if (::connect(socket, reinterpret_cast<sockaddr *>(&address),
                sizeof(address)) != 0) {
    std::cerr << "Could not connect to UniDB at " << destination << ':' << port
              << ": " << std::strerror(errno) << '\n';
    ::close(socket);
    return -1;
  }
  return socket;
}

int startServer(const char *executable, std::string_view databasePath,
                unsigned int port) {
  const std::string bindAddress = serverBindAddress();
  const std::string conflictProbeAddress = serverProbeAddress(bindAddress);
  in_addr configuredBindAddress{};
  if (::inet_pton(AF_INET, bindAddress.c_str(), &configuredBindAddress) != 1) {
    std::cerr << "Invalid IPv4 bind address '" << bindAddress << "'.\n";
    return 2;
  }
  in_addr conflictProbeAddressValue{};
  if (::inet_pton(AF_INET, conflictProbeAddress.c_str(),
                  &conflictProbeAddressValue) != 1) {
    std::cerr << "Invalid IPv4 probe address '" << conflictProbeAddress
              << "'.\n";
    return 2;
  }
#if !defined(__APPLE__) && !defined(__linux__)
  (void)executable;
  (void)databasePath;
  (void)port;
  (void)bindAddress;
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
    std::string recordedBindAddress;
    if (pidFile >> existingPid >> recordedPort) {
      const bool validPid =
          existingPid > 1 &&
          existingPid <= static_cast<long>(std::numeric_limits<pid_t>::max());
      const auto existingProcess = static_cast<pid_t>(existingPid);
      const bool processAlive =
          validPid && (::kill(existingProcess, 0) == 0 || errno == EPERM);
      if (processAlive) {
        // Older UniDB versions wrote PID, port, and process identity only.
        // Those servers always bound to loopback, so preserve compatibility.
        if (!(pidFile >> recordedIdentity) || recordedIdentity.empty()) {
          std::cerr
              << "PID file refers to a live process but is missing its "
                 "process identity; refusing to remove it automatically.\n";
          return 1;
        }
        if (!(pidFile >> recordedBindAddress))
          recordedBindAddress = "127.0.0.1";
        if (recordedBindAddress.empty()) {
          std::cerr << "PID file contains an empty bind address; refusing to "
                       "remove it automatically.\n";
          return 1;
        }
        const std::string currentIdentity = processIdentity(existingProcess);
        if (currentIdentity.empty()) {
          std::cerr << "Could not verify the live process identity in "
                    << pidPath << "; refusing to remove the PID file.\n";
          return 1;
        }
        if (currentIdentity == recordedIdentity) {
          if (recordedPort == port &&
              isPortReachable(recordedPort, recordedBindAddress)) {
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
    existingAddress.sin_addr = conflictProbeAddressValue;
    const bool portInUse =
        ::connect(existingListener,
                  reinterpret_cast<sockaddr *>(&existingAddress),
                  sizeof(existingAddress)) == 0;
    ::close(existingListener);
    if (portInUse) {
      std::cerr << "Port " << port << " is already in use on " << bindAddress
                << ".\n";
      return 1;
    }
  }

  // Keep the child blocked until its PID record is written. If this launcher
  // exits before releasing it, EOF makes the child exit before opening the DB.
  int startupPipe[2] = {-1, -1};
  int readinessPipe[2] = {-1, -1};
  if (::pipe(startupPipe) != 0 || ::pipe(readinessPipe) != 0) {
    if (startupPipe[0] >= 0) {
      ::close(startupPipe[0]);
      ::close(startupPipe[1]);
    }
    if (readinessPipe[0] >= 0) {
      ::close(readinessPipe[0]);
      ::close(readinessPipe[1]);
    }
    std::cerr << "Could not create UniDB startup handshake: "
              << std::strerror(errno) << '\n';
    return 1;
  }

  const pid_t child = ::fork();
  if (child < 0) {
    const int forkError = errno;
    ::close(startupPipe[0]);
    ::close(startupPipe[1]);
    ::close(readinessPipe[0]);
    ::close(readinessPipe[1]);
    std::cerr << "Could not start UniDB: " << std::strerror(forkError) << '\n';
    return 1;
  }
  if (child == 0) {
    ::close(startupPipe[1]);
    ::close(readinessPipe[0]);
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
        ::open(logPath.c_str(),
               O_WRONLY | O_CREAT | O_APPEND | O_NOFOLLOW | O_CLOEXEC, 0644);
    if (logFd < 0) {
      static constexpr char message[] =
          "UniDB could not open the log file; symlinks are rejected.\n";
      (void)::write(STDERR_FILENO, message, sizeof(message) - 1);
      _exit(126);
    }
    struct stat logInfo{};
    if (::fstat(logFd, &logInfo) != 0) {
      static constexpr char message[] =
          "UniDB could not inspect the log file.\n";
      (void)::write(STDERR_FILENO, message, sizeof(message) - 1);
      ::close(logFd);
      _exit(126);
    }
    if (!S_ISREG(logInfo.st_mode)) {
      static constexpr char message[] =
          "UniDB log path is not a regular file.\n";
      (void)::write(STDERR_FILENO, message, sizeof(message) - 1);
      ::close(logFd);
      _exit(126);
    }
    ::dup2(logFd, STDOUT_FILENO);
    ::dup2(logFd, STDERR_FILENO);
    if (logFd > STDERR_FILENO)
      ::close(logFd);

    const std::string databaseArg(databasePath);
    const std::string portArg = std::to_string(port);
    const std::string readinessFd = std::to_string(readinessPipe[1]);
    if (::setenv("UNIDB_READY_FD", readinessFd.c_str(), 1) != 0)
      _exit(126);
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
  ::close(readinessPipe[1]);
  // Prevent a broken handshake pipe from terminating the launcher before
  // cleanup.
  std::signal(SIGPIPE, SIG_IGN);

  {
    const std::string identity = processIdentity(child);
    const int pidFd =
        identity.empty()
            ? -1
            : ::open(pidPath.c_str(),
                     O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC,
                     0644);
    bool pidFileWritten = pidFd >= 0;
    if (pidFileWritten) {
      struct stat pidInfo{};
      pidFileWritten =
          ::fstat(pidFd, &pidInfo) == 0 && S_ISREG(pidInfo.st_mode);
    }

    const std::string pidRecord = std::to_string(child) + '\n' +
                                  std::to_string(port) + '\n' + identity +
                                  '\n' + bindAddress + '\n';
    std::size_t written = 0;
    while (pidFileWritten && written < pidRecord.size()) {
      const auto count = ::write(pidFd, pidRecord.data() + written,
                                 pidRecord.size() - written);
      if (count < 0 && errno == EINTR)
        continue;
      if (count <= 0) {
        pidFileWritten = false;
        break;
      }
      written += static_cast<std::size_t>(count);
    }
    if (pidFd >= 0 && ::close(pidFd) != 0)
      pidFileWritten = false;

    if (!pidFileWritten) {
      std::cerr << "Could not securely create or write PID file: " << pidPath
                << '\n';
      ::close(startupPipe[1]);
      ::close(readinessPipe[0]);
      ::close(readinessPipe[1]);
      ::kill(child, SIGTERM);
      ::waitpid(child, nullptr, 0);
      return 1;
    }
  }

  // The complete PID record is written and closed before the child can
  // initialize storage.
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

  // Readiness is reported only after the child successfully calls listen().
  pollfd readinessEvent{readinessPipe[0], POLLIN | POLLHUP, 0};
  int pollResult = -1;
  do {
    pollResult = ::poll(&readinessEvent, 1, 5000);
  } while (pollResult < 0 && errno == EINTR);
  char readySignal = 0;
  ssize_t readyBytes = -1;
  if (pollResult > 0 && (readinessEvent.revents & POLLIN)) {
    do {
      readyBytes = ::read(readinessPipe[0], &readySignal, 1);
    } while (readyBytes < 0 && errno == EINTR);
  }
  ::close(readinessPipe[0]);
  if (readyBytes == 1 && readySignal == 'R') {
    std::cout << "UniDB started (PID " << child << ") on " << bindAddress << ':'
              << port << "\nLog: " << logPath << '\n';
    return 0;
  }
  std::cerr << "Timed out waiting for UniDB to listen. Check the log: "
            << logPath << '\n';
  ::kill(child, SIGTERM);
  ::waitpid(child, nullptr, 0);
  std::remove(pidPath.c_str());
  return 1;
#endif
}

int stopServer(std::string_view databasePath, unsigned int port,
               bool portSpecified) {
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
  std::string recordedBindAddress;
  if (!(pidFile >> rawPid >> recordedPort >> recordedIdentity) || rawPid <= 1 ||
      recordedPort == 0 || recordedPort > 65535 || recordedIdentity.empty()) {
    std::cerr << "UniDB PID file not found or invalid: " << pidPath << '\n';
    return 1;
  }
  // Legacy three-field PID records imply the historical loopback bind.
  if (!(pidFile >> recordedBindAddress))
    recordedBindAddress = "127.0.0.1";
  if (recordedBindAddress.empty()) {
    std::cerr << "UniDB PID file contains an empty bind address: " << pidPath
              << '\n';
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
  if (portSpecified && recordedPort != port) {
    std::cerr << "UniDB was started on port " << recordedPort
              << "; rerun stop with --port " << recordedPort << ".\n";
    return 1;
  }
  if (!isPortReachable(recordedPort, recordedBindAddress)) {
    std::cerr << "The UniDB listener recorded in " << pidPath
              << " is not reachable on port " << recordedPort
              << "; refusing to signal PID " << rawPid << ".\n";
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

int connectInteractive(unsigned int port, std::string_view destination,
                       std::string_view tlsCaFile,
                       std::string_view tlsCertificateFile,
                       std::string_view tlsPrivateKeyFile) {
  // Ignore SIGPIPE so a disconnected server is handled through send errors.
  std::signal(SIGPIPE, SIG_IGN);
  const int socket = connectToServer(port, destination);
  if (socket < 0)
    return 1;

  using SslContextPtr = std::unique_ptr<SSL_CTX, decltype(&SSL_CTX_free)>;
  using SslSessionPtr = std::unique_ptr<SSL, decltype(&SSL_free)>;
  SslContextPtr tlsContext(nullptr, SSL_CTX_free);
  SslSessionPtr tls(nullptr, SSL_free);
  if (!tlsCaFile.empty()) {
    tlsContext.reset(SSL_CTX_new(TLS_client_method()));
    if (!tlsContext ||
        SSL_CTX_set_min_proto_version(tlsContext.get(), TLS1_2_VERSION) != 1 ||
        SSL_CTX_load_verify_locations(
            tlsContext.get(), std::string(tlsCaFile).c_str(), nullptr) != 1 ||
        SSL_CTX_use_certificate_chain_file(
            tlsContext.get(), std::string(tlsCertificateFile).c_str()) != 1 ||
        SSL_CTX_use_PrivateKey_file(tlsContext.get(),
                                    std::string(tlsPrivateKeyFile).c_str(),
                                    SSL_FILETYPE_PEM) != 1 ||
        SSL_CTX_check_private_key(tlsContext.get()) != 1) {
      std::cerr << "Could not configure mutual TLS client credentials.\n";
      ERR_print_errors_fp(stderr);
      ::close(socket);
      return 1;
    }
    SSL_CTX_set_verify(tlsContext.get(), SSL_VERIFY_PEER, nullptr);
    tls.reset(SSL_new(tlsContext.get()));
    const std::string destinationAddress(destination);
    if (!tls || SSL_set_fd(tls.get(), socket) != 1 ||
        X509_VERIFY_PARAM_set1_ip_asc(SSL_get0_param(tls.get()),
                                      destinationAddress.c_str()) != 1) {
      std::cerr << "Could not initialize mutual TLS client session.\n";
      ERR_print_errors_fp(stderr);
      ::close(socket);
      return 1;
    }
    // A queued SSL_write retry may follow a string reallocation. OpenSSL still
    // requires the same write length and bytes, but permits a moved pointer.
    SSL_set_mode(tls.get(), SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER |
                                SSL_MODE_ENABLE_PARTIAL_WRITE);
    if (SSL_connect(tls.get()) != 1) {
      std::cerr << "Mutual TLS connection to UniDB failed. Check the server "
                   "certificate and client credentials.\n";
      ERR_print_errors_fp(stderr);
      ::close(socket);
      return 1;
    }
  }

  // Complete the TLS handshake while the socket is blocking, then use poll
  // for all I/O so a full send buffer cannot stall response processing.
  const int socketFlags = ::fcntl(socket, F_GETFL, 0);
  if (socketFlags < 0 ||
      ::fcntl(socket, F_SETFL, socketFlags | O_NONBLOCK) < 0) {
    std::cerr << "Could not make the UniDB client socket nonblocking: "
              << std::strerror(errno) << '\n';
    ::close(socket);
    return 1;
  }

  std::cout << "Connected to UniDB at " << destination << ':' << port
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

  // OpenSSL needs the exact same write length for a WANT_READ/WANT_WRITE
  // retry. Keep this length until SSL_write succeeds or fails permanently.
  bool tlsWriteInFlight = false;
  std::size_t tlsWriteLength = 0;
  short tlsWriteWaitEvents = POLLOUT;
  short tlsReadWaitEvents = POLLIN;
  bool tlsShutdownPending = false;
  short tlsShutdownWaitEvents = POLLOUT;

  auto writeToServer = [&](const char *data, std::size_t size) -> ssize_t {
    if (!tls)
      return ::send(socket, data, size, 0);

    if (!tlsWriteInFlight) {
      tlsWriteInFlight = true;
      tlsWriteLength = size;
    } else if (size != tlsWriteLength) {
      errno = EINVAL;
      return -1;
    }

    const int count = SSL_write(tls.get(), data,
                                static_cast<int>(tlsWriteLength));
    if (count > 0) {
      tlsWriteInFlight = false;
      tlsWriteLength = 0;
      tlsWriteWaitEvents = POLLOUT;
      return count;
    }

    const int sslError = SSL_get_error(tls.get(), count);
    if (sslError == SSL_ERROR_WANT_READ) {
      tlsWriteWaitEvents = POLLIN;
      errno = EAGAIN;
      return -1;
    }
    if (sslError == SSL_ERROR_WANT_WRITE) {
      tlsWriteWaitEvents = POLLOUT;
      errno = EAGAIN;
      return -1;
    }
    if (sslError == SSL_ERROR_SYSCALL && errno == EINTR) {
      tlsWriteWaitEvents = POLLOUT;
      return -1;
    }

    tlsWriteInFlight = false;
    tlsWriteLength = 0;
    errno = EPIPE;
    return -1;
  };

  auto readFromServer = [&](char *data, std::size_t size) -> ssize_t {
    if (!tls)
      return ::recv(socket, data, size, 0);

    const int count = SSL_read(tls.get(), data, static_cast<int>(size));
    if (count > 0) {
      tlsReadWaitEvents = POLLIN;
      return count;
    }

    const int sslError = SSL_get_error(tls.get(), count);
    if (sslError == SSL_ERROR_ZERO_RETURN)
      return 0;
    if (sslError == SSL_ERROR_WANT_READ) {
      tlsReadWaitEvents = POLLIN;
      errno = EAGAIN;
      return -1;
    }
    if (sslError == SSL_ERROR_WANT_WRITE) {
      tlsReadWaitEvents = POLLOUT;
      errno = EAGAIN;
      return -1;
    }
    if (sslError == SSL_ERROR_SYSCALL && errno == EINTR) {
      tlsReadWaitEvents = POLLIN;
      return -1;
    }

    errno = ECONNRESET;
    return -1;
  };

  while (true) {
    const std::size_t pendingBytes = pendingInput.size() - pendingOffset;
    descriptors[0] = {
        stdinOpen && pendingBytes < kMaxPendingInput ? STDIN_FILENO : -1,
        POLLIN, 0};

    // Monitor the event OpenSSL actually requested for an in-flight operation.
    // Always monitor the read side too, so response traffic can drain while a
    // large piped input is waiting for socket write capacity.
    short socketEvents = tls ? tlsReadWaitEvents : POLLIN;
    if (pendingBytes > 0) {
      socketEvents |=
          (tls && tlsWriteInFlight) ? tlsWriteWaitEvents : POLLOUT;
    }
    const bool shouldShutdownTls =
        tls && !stdinOpen && pendingBytes == 0 && !writeShutdown;
    if (shouldShutdownTls) {
      socketEvents |=
          tlsShutdownPending ? tlsShutdownWaitEvents : POLLOUT;
    }
    descriptors[1] = {socket, socketEvents, 0};

    // SSL_pending() counts decrypted bytes already buffered by OpenSSL. They
    // must be consumed even when the underlying file descriptor is not readable.
    const bool bufferedTlsData = tls && SSL_pending(tls.get()) > 0;
    const int ready = ::poll(descriptors, 2, bufferedTlsData ? 0 : -1);
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

    const std::size_t pendingBytesNow = pendingInput.size() - pendingOffset;
    const short requiredWriteEvents =
        (tls && tlsWriteInFlight) ? tlsWriteWaitEvents : POLLOUT;
    if (pendingBytesNow > 0 &&
        (descriptors[1].revents & requiredWriteEvents)) {
      const std::size_t bytesToWrite =
          (tls && tlsWriteInFlight) ? tlsWriteLength : pendingBytesNow;
      const auto count =
          writeToServer(pendingInput.data() + pendingOffset, bytesToWrite);
      if (count > 0) {
        pendingOffset += static_cast<std::size_t>(count);
        if (pendingOffset == pendingInput.size()) {
          pendingInput.clear();
          pendingOffset = 0;
        } else if (pendingOffset >= kMaxPendingInput / 2) {
          // No SSL_write is outstanding after a positive write result, so
          // compacting the queued buffer cannot invalidate a pending retry.
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

    // Retry a nonblocking TLS shutdown only when its previous WANT_* event is
    // ready. The first call sends close_notify after all queued SQL is sent.
    if (!stdinOpen && pendingInput.empty() && !writeShutdown) {
      const bool shutdownReady =
          !tlsShutdownPending ||
          (descriptors[1].revents & tlsShutdownWaitEvents) != 0;
      if (!tls || shutdownReady) {
        if (!tls) {
          ::shutdown(socket, SHUT_WR);
          writeShutdown = true;
        } else {
          const int shutdownResult = SSL_shutdown(tls.get());
          if (shutdownResult >= 0) {
            writeShutdown = true;
            tlsShutdownPending = false;
          } else {
            const int sslError = SSL_get_error(tls.get(), shutdownResult);
            if (sslError == SSL_ERROR_WANT_READ) {
              tlsShutdownPending = true;
              tlsShutdownWaitEvents = POLLIN;
            } else if (sslError == SSL_ERROR_WANT_WRITE) {
              tlsShutdownPending = true;
              tlsShutdownWaitEvents = POLLOUT;
            } else if (sslError == SSL_ERROR_SYSCALL && errno == EINTR) {
              tlsShutdownPending = true;
              tlsShutdownWaitEvents = POLLOUT;
            } else {
              std::cerr << "Failed to close the TLS write side cleanly.\n";
              ERR_print_errors_fp(stderr);
              ::close(socket);
              return 1;
            }
          }
        }
      }
    }

    const short socketErrorEvents = POLLHUP | POLLERR | POLLNVAL;
    const bool socketHasError =
        (descriptors[1].revents & socketErrorEvents) != 0;
    const bool readReady =
        bufferedTlsData ||
        (descriptors[1].revents & (tls ? tlsReadWaitEvents : POLLIN)) != 0 ||
        socketHasError;
    if (readReady) {
      const auto count = readFromServer(buffer, sizeof(buffer));
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
  bool portSpecified = false;
  std::string_view host = "127.0.0.1";
  std::string_view tlsCaFile;
  std::string_view tlsCertificateFile;
  std::string_view tlsPrivateKeyFile;

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
        portSpecified = true;
      } catch (const std::exception &) {
        std::cerr << "Invalid TCP port. Expected a number from 1 to 65535.\n";
        return 2;
      }
    } else if (argument == "--host") {
      if (index + 1 >= argc) {
        std::cerr << "Missing IPv4 address after --host.\n";
        return 2;
      }
      host = argv[++index];
    } else if (argument == "--tls-ca-file") {
      if (index + 1 >= argc) {
        std::cerr << "Missing path after --tls-ca-file.\n";
        return 2;
      }
      tlsCaFile = argv[++index];
    } else if (argument == "--tls-cert-file") {
      if (index + 1 >= argc) {
        std::cerr << "Missing path after --tls-cert-file.\n";
        return 2;
      }
      tlsCertificateFile = argv[++index];
    } else if (argument == "--tls-key-file") {
      if (index + 1 >= argc) {
        std::cerr << "Missing path after --tls-key-file.\n";
        return 2;
      }
      tlsPrivateKeyFile = argv[++index];
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
             "[--port PORT] [--host IPv4] [--tls-ca-file PATH] "
             "[--tls-cert-file PATH] [--tls-key-file PATH]\n"
             "  start          Start a background server and write a PID/log "
             "file.\n"
             "  connect        Open an interactive SQL connection to the "
             "server.\n"
             "  stop           Request graceful shutdown and wait for exit.\n"
             "  --server       Run the TCP server in the foreground (legacy "
             "mode).\n"
             "  --db-path PATH Database file path (default: database.db).\n"
             "  --port PORT    TCP port (default: 7432).\n"
             "  --host IPv4    Server address for connect (default: "
             "127.0.0.1).\n"
             "  --tls-ca-file PATH   CA certificate to verify the TLS server.\n"
             "  --tls-cert-file PATH Client certificate for mutual TLS.\n"
             "  --tls-key-file PATH  Private key for the client certificate.\n";
      return 0;
    } else {
      std::cerr << "Unknown argument: " << argument << '\n'
                << "Use --help for usage.\n";
      return 2;
    }
  }

  if (command == "start")
    return startServer(argv[0], databasePath, port);
  if (command == "connect") {
    const bool hasCa = !tlsCaFile.empty();
    const bool hasCert = !tlsCertificateFile.empty();
    const bool hasKey = !tlsPrivateKeyFile.empty();
    if (hasCa != hasCert || hasCa != hasKey) {
      std::cerr << "TLS connections require --tls-ca-file, --tls-cert-file, "
                   "and --tls-key-file together.\n";
      return 2;
    }
    if (!isLoopbackIpv4(host) && !hasCa) {
      std::cerr << "Remote UniDB connections require mutual TLS. Supply "
                   "--tls-ca-file, --tls-cert-file, and --tls-key-file.\n";
      return 2;
    }
    return connectInteractive(port, host, tlsCaFile, tlsCertificateFile,
                              tlsPrivateKeyFile);
  }
  if (command == "stop")
    return stopServer(databasePath, port, portSpecified);

  DatabaseOwnershipLock databaseOwnershipLock(databasePath);
  if (!databaseOwnershipLock.acquired()) {
    if (databaseOwnershipLock.contended()) {
      std::cerr
          << "Another UniDB process already owns this database: "
          << databasePath << '\n'
          << "Stop the other process before opening this database again.\n";
    } else {
      std::cerr << "Could not acquire database ownership: "
                << databaseOwnershipLock.failureReason() << '\n';
    }
    return 1;
  }

  try {
    Container container(std::string(databasePath), 2, EvictionPolicyType::FIFO);

    const bool initializedNewDatabase = DatabaseStartup::execute(container);
    if (!databaseOwnershipLock.acquireCreatedDatabaseLock()) {
      std::cerr << "Could not retain exclusive ownership of database: "
                << databasePath << '\n';
      return 1;
    }
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
