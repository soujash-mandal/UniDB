#include "SqlConnectionServer.h"

#include "../QueryExecutionService/actions/ExecuteQueryAction.h"
#include "../container/container.h"
#include "SqlRequestFramer.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <variant>
#include <vector>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <openssl/err.h>
#include <openssl/ssl.h>

namespace ConnectionProtocol {
namespace {
volatile std::sig_atomic_t stopRequested = 0;
extern "C" void requestStop(int) { stopRequested = 1; }

std::string jsonString(std::string_view value) {
  std::string result = "\"";
  for (const unsigned char ch : value) {
    switch (ch) {
    case '"':
      result += "\\\"";
      break;
    case '\\':
      result += "\\\\";
      break;
    case '\n':
      result += "\\n";
      break;
    case '\r':
      result += "\\r";
      break;
    case '\t':
      result += "\\t";
      break;
    default:
      if (ch < 0x20) {
        constexpr char digits[] = "0123456789abcdef";
        result += "\\u00";
        result += digits[ch >> 4];
        result += digits[ch & 0x0f];
      } else {
        result += static_cast<char>(ch);
      }
    }
  }
  result += '"';
  return result;
}

std::string
responseJson(const QueryExecutionService::QueryExecutionOutcome &outcome) {
  using namespace QueryExecutionService;
  if (const auto *error = std::get_if<QueryExecutionError>(&outcome)) {
    const char *code = "execution_error";
    if (error->code == QueryExecutionErrorCode::SyntaxError)
      code = "syntax_error";
    if (error->code == QueryExecutionErrorCode::UnsupportedStatement)
      code = "unsupported_statement";
    return "{\"ok\":false,\"error\":{\"code\":" + jsonString(code) +
           ",\"message\":" + jsonString(error->message) + "}}\n";
  }

  const auto &result = std::get<QueryExecutionResult>(outcome);
  if (result.kind == QueryExecutionResultKind::Command) {
    return "{\"ok\":true,\"kind\":\"command\",\"message\":" +
           jsonString(result.message) +
           ",\"affected_rows\":" + std::to_string(result.affectedRows) + "}\n";
  }

  std::string response = "{\"ok\":true,\"kind\":\"rows\",\"columns\":[";
  for (std::size_t i = 0; i < result.columns.size(); ++i) {
    if (i)
      response += ',';
    response += "{\"name\":" + jsonString(result.columns[i].name) +
                ",\"type\":" +
                jsonString(result.columns[i].type == unidb::sql::SqlType::Int
                               ? "int"
                               : "text") +
                "}";
  }
  response += "],\"rows\":[";
  for (std::size_t row = 0; row < result.rows.size(); ++row) {
    if (row)
      response += ',';
    response += '[';
    for (std::size_t col = 0; col < result.rows[row].size(); ++col) {
      if (col)
        response += ',';
      const auto &value = result.rows[row][col];
      if (const auto *integer = std::get_if<std::int64_t>(&value)) {
        response += std::to_string(*integer);
      } else {
        response += jsonString(std::get<std::string>(value));
      }
    }
    response += ']';
  }
  response += "]}\n";
  return response;
}

bool sendAll(int socket, SSL *tls, std::string_view bytes) {
  while (!bytes.empty()) {
    if (stopRequested)
      return false;
    if (tls != nullptr) {
      const auto maxChunk =
          static_cast<std::size_t>(std::numeric_limits<int>::max());
      const int chunkSize =
          static_cast<int>(bytes.size() > maxChunk ? maxChunk : bytes.size());
      const int sent = SSL_write(tls, bytes.data(), chunkSize);
      if (sent <= 0) {
        const int sslError = SSL_get_error(tls, sent);
        if (sslError == SSL_ERROR_SYSCALL && errno == EINTR && !stopRequested)
          continue;
        return false;
      }
      bytes.remove_prefix(static_cast<std::size_t>(sent));
      continue;
    }
    const auto sent = ::send(socket, bytes.data(), bytes.size(), 0);
    if (sent < 0) {
      if (errno == EINTR && !stopRequested)
        continue;
      return false;
    }
    if (sent == 0)
      return false;
    bytes.remove_prefix(static_cast<std::size_t>(sent));
  }
  return true;
}

void serveClient(int client, Container &container, SSL *tls,
                 std::mutex &containerMutex) {
  SqlRequestFramer framer;
  char buffer[4096];
  while (!stopRequested) {
    int received = 0;
    if (tls != nullptr) {
      received = SSL_read(tls, buffer, static_cast<int>(sizeof(buffer)));
      if (received <= 0) {
        const int sslError = SSL_get_error(tls, received);
        if (sslError == SSL_ERROR_ZERO_RETURN)
          break;
        if (sslError == SSL_ERROR_SYSCALL && errno == EINTR)
          continue;
        break;
      }
    } else {
      const auto count = ::recv(client, buffer, sizeof(buffer), 0);
      if (count == 0)
        break;
      if (count < 0) {
        if (errno == EINTR)
          continue;
        break;
      }
      received = static_cast<int>(count);
    }

    auto framing = framer.append(
        std::string_view(buffer, static_cast<std::size_t>(received)));
    for (const auto &sql : framing.requests) {
      std::string response;
      {
        // Container and its service actions are shared by all client threads.
        // Keep database operations serialized, but never hold this lock during
        // network reads or writes from an individual client.
        std::lock_guard<std::mutex> lock(containerMutex);
        response = responseJson(container.executeQueryAction().execute(sql));
      }
      if (!sendAll(client, tls, response))
        return;
    }
    if (framing.error) {
      const std::string response =
          "{\"ok\":false,\"error\":{\"code\":\"invalid_request\",\"message\":" +
          jsonString(*framing.error) + "}}\n";
      sendAll(client, tls, response);
      break;
    }
  }
}

using SslContextPtr = std::unique_ptr<SSL_CTX, decltype(&SSL_CTX_free)>;

struct ClientConnection {
  int socket;
  SSL *tls;
};

struct PendingTlsHandshake {
  int socket;
  SSL *tls;
  int originalFlags;
  std::uint32_t peerAddress;
  std::chrono::steady_clock::time_point deadline;
  short events;
};

void closeClientConnection(const ClientConnection &connection) {
  if (connection.tls != nullptr)
    SSL_free(connection.tls);
  if (connection.socket >= 0)
    ::close(connection.socket);
}

void closePendingTlsHandshake(const PendingTlsHandshake &handshake) {
  if (handshake.tls != nullptr)
    SSL_free(handshake.tls);
  if (handshake.socket >= 0)
    ::close(handshake.socket);
  ERR_clear_error();
}

bool hasTrustedClientCertificate(SSL *tls) {
  X509 *peerCertificate = SSL_get1_peer_certificate(tls);
  const long verificationResult = SSL_get_verify_result(tls);
  const bool trusted =
      peerCertificate != nullptr && verificationResult == X509_V_OK;
  if (peerCertificate != nullptr)
    X509_free(peerCertificate);
  return trusted;
}

// Accept new sockets while progressing all pending TLS handshakes in one poll
// loop. Authenticated sessions are queued for independent client workers, so
// an idle client cannot block admission of later connections.
void acceptAndAuthenticateClients(int listener, SSL_CTX *tlsContext,
                                  std::mutex &sessionsMutex,
                                  std::condition_variable &sessionsReady,
                                  std::deque<ClientConnection> &readySessions,
                                  bool &acceptFailed,
                                  bool &acceptLoopFinished) {
  constexpr std::size_t maximumPendingHandshakes = 32;
  constexpr std::size_t maximumPendingHandshakesPerAddress = 4;
  constexpr std::size_t maximumReadySessions = 16;
  constexpr auto handshakeTimeout = std::chrono::seconds(5);
  constexpr int pollIntervalMilliseconds = 100;

  std::vector<PendingTlsHandshake> pendingHandshakes;

  const auto queueSession = [&](const ClientConnection &connection) {
    std::lock_guard<std::mutex> lock(sessionsMutex);
    if (stopRequested || readySessions.size() >= maximumReadySessions) {
      std::cerr << "Rejected SQL client: too many sessions are waiting to be "
                   "served.\n";
      closeClientConnection(connection);
      return false;
    }
    readySessions.push_back(connection);
    sessionsReady.notify_one();
    return true;
  };

  while (!stopRequested) {
    const std::size_t polledPendingCount = pendingHandshakes.size();
    std::vector<pollfd> descriptors;
    descriptors.reserve(polledPendingCount + 1);
    descriptors.push_back(pollfd{listener, POLLIN, 0});
    for (std::size_t i = 0; i < polledPendingCount; ++i) {
      descriptors.push_back(
          pollfd{pendingHandshakes[i].socket, pendingHandshakes[i].events, 0});
    }

    const int pollResult = ::poll(descriptors.data(), descriptors.size(),
                                  pollIntervalMilliseconds);
    if (pollResult < 0) {
      if (errno == EINTR)
        continue;
      if (!stopRequested) {
        std::cerr << "UniDB accept poll failed: " << std::strerror(errno)
                  << '\n';
        acceptFailed = true;
      }
      break;
    }

    const short listenerEvents = descriptors.front().revents;
    if ((listenerEvents & (POLLERR | POLLHUP | POLLNVAL)) != 0 &&
        (listenerEvents & POLLIN) == 0) {
      std::cerr << "UniDB listener reported a poll error.\n";
      acceptFailed = true;
      break;
    }

    if ((listenerEvents & POLLIN) != 0) {
      sockaddr_in peerAddress{};
      socklen_t peerLength = sizeof(peerAddress);
      const int client = ::accept(
          listener, reinterpret_cast<sockaddr *>(&peerAddress), &peerLength);
      if (client < 0) {
        if (errno != EINTR && errno != ECONNABORTED && errno != EAGAIN &&
            errno != EWOULDBLOCK && !stopRequested) {
          std::cerr << "UniDB accept failed: " << std::strerror(errno) << '\n';
          acceptFailed = true;
          break;
        }
      } else if (tlsContext == nullptr) {
        (void)queueSession(ClientConnection{client, nullptr});
      } else {
        const auto pendingFromAddress = std::count_if(
            pendingHandshakes.begin(), pendingHandshakes.end(),
            [&](const PendingTlsHandshake &handshake) {
              return handshake.peerAddress == peerAddress.sin_addr.s_addr;
            });
        if (pendingHandshakes.size() >= maximumPendingHandshakes) {
          std::cerr
              << "Rejected SQL client: too many TLS handshakes are pending.\n";
          ::close(client);
        } else if (pendingFromAddress >= maximumPendingHandshakesPerAddress) {
          std::cerr << "Rejected SQL client: pending TLS handshake limit for "
                       "this address reached.\n";
          ::close(client);
        } else {
          const int originalFlags = ::fcntl(client, F_GETFL, 0);
          if (originalFlags < 0 ||
              ::fcntl(client, F_SETFL, originalFlags | O_NONBLOCK) != 0) {
            std::cerr << "Could not configure TLS client socket.\n";
            ::close(client);
          } else {
            SSL *tls = SSL_new(tlsContext);
            if (tls == nullptr || SSL_set_fd(tls, client) != 1) {
              std::cerr
                  << "Could not initialize TLS for an incoming SQL client.\n";
              ERR_print_errors_fp(stderr);
              if (tls != nullptr)
                SSL_free(tls);
              ::close(client);
            } else {
              pendingHandshakes.push_back(PendingTlsHandshake{
                  client, tls, originalFlags, peerAddress.sin_addr.s_addr,
                  std::chrono::steady_clock::now() + handshakeTimeout, POLLIN});
            }
          }
        }
      }
    }

    const auto now = std::chrono::steady_clock::now();
    for (std::size_t reverse = polledPendingCount; reverse > 0; --reverse) {
      const std::size_t index = reverse - 1;
      auto &handshake = pendingHandshakes[index];
      const short events = descriptors[index + 1].revents;
      if (now >= handshake.deadline ||
          (events & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
        std::cerr << "Rejected SQL client: mutual TLS handshake failed.\n";
        closePendingTlsHandshake(handshake);
        pendingHandshakes.erase(pendingHandshakes.begin() + index);
        continue;
      }
      if ((events & handshake.events) == 0)
        continue;

      const int result = SSL_accept(handshake.tls);
      if (result == 1) {
        if (!hasTrustedClientCertificate(handshake.tls)) {
          std::cerr << "Rejected SQL client: a trusted client certificate is "
                       "required.\n";
          closePendingTlsHandshake(handshake);
          pendingHandshakes.erase(pendingHandshakes.begin() + index);
          continue;
        }
        if (::fcntl(handshake.socket, F_SETFL, handshake.originalFlags) != 0) {
          std::cerr << "Could not restore TLS client socket flags.\n";
          closePendingTlsHandshake(handshake);
          pendingHandshakes.erase(pendingHandshakes.begin() + index);
          continue;
        }

        const ClientConnection connection{handshake.socket, handshake.tls};
        handshake.socket = -1;
        handshake.tls = nullptr;
        pendingHandshakes.erase(pendingHandshakes.begin() + index);
        (void)queueSession(connection);
        continue;
      }

      const int sslError = SSL_get_error(handshake.tls, result);
      if (sslError == SSL_ERROR_WANT_READ) {
        handshake.events = POLLIN;
      } else if (sslError == SSL_ERROR_WANT_WRITE) {
        handshake.events = POLLOUT;
      } else {
        std::cerr << "Rejected SQL client: mutual TLS handshake failed.\n";
        closePendingTlsHandshake(handshake);
        pendingHandshakes.erase(pendingHandshakes.begin() + index);
      }
    }
  }

  for (const auto &handshake : pendingHandshakes)
    closePendingTlsHandshake(handshake);

  {
    std::lock_guard<std::mutex> lock(sessionsMutex);
    for (const auto &connection : readySessions)
      closeClientConnection(connection);
    readySessions.clear();
    acceptLoopFinished = true;
  }
  sessionsReady.notify_all();
}

SslContextPtr createMutualTlsContext() {
  const char *certificateFile = std::getenv("UNIDB_TLS_CERT_FILE");
  const char *privateKeyFile = std::getenv("UNIDB_TLS_KEY_FILE");
  const char *caFile = std::getenv("UNIDB_TLS_CA_FILE");
  if (certificateFile == nullptr || certificateFile[0] == '\0' ||
      privateKeyFile == nullptr || privateKeyFile[0] == '\0' ||
      caFile == nullptr || caFile[0] == '\0') {
    std::cerr << "Non-loopback SQL listeners require mutual TLS. Set "
                 "UNIDB_TLS_CERT_FILE, UNIDB_TLS_KEY_FILE, and "
                 "UNIDB_TLS_CA_FILE.\n";
    return SslContextPtr(nullptr, SSL_CTX_free);
  }

  SslContextPtr context(SSL_CTX_new(TLS_server_method()), SSL_CTX_free);
  if (!context) {
    std::cerr << "Could not create TLS context.\n";
    ERR_print_errors_fp(stderr);
    return context;
  }
  if (SSL_CTX_set_min_proto_version(context.get(), TLS1_2_VERSION) != 1 ||
      SSL_CTX_use_certificate_chain_file(context.get(), certificateFile) != 1 ||
      SSL_CTX_use_PrivateKey_file(context.get(), privateKeyFile,
                                  SSL_FILETYPE_PEM) != 1 ||
      SSL_CTX_check_private_key(context.get()) != 1 ||
      SSL_CTX_load_verify_locations(context.get(), caFile, nullptr) != 1) {
    std::cerr << "Could not configure mutual TLS; check the certificate, "
                 "private key, and client CA files.\n";
    ERR_print_errors_fp(stderr);
    return SslContextPtr(nullptr, SSL_CTX_free);
  }

  SSL_CTX_set_verify(context.get(),
                     SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
                     nullptr);
  return context;
}
} // namespace

SqlConnectionServer::SqlConnectionServer(Container &container,
                                         std::uint16_t port,
                                         std::string bindAddress)
    : container(container), port(port), bindAddress(std::move(bindAddress)) {
  if (this->bindAddress.empty()) {
    const char *configuredAddress = std::getenv("UNIDB_BIND_ADDRESS");
    this->bindAddress =
        (configuredAddress == nullptr || configuredAddress[0] == '\0')
            ? "127.0.0.1"
            : configuredAddress;
  }
}

int SqlConnectionServer::run() {
  struct sigaction action{};
  action.sa_handler = requestStop;
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;
  if (sigaction(SIGINT, &action, nullptr) != 0 ||
      sigaction(SIGTERM, &action, nullptr) != 0) {
    std::cerr << "UniDB failed to install shutdown handlers\n";
    return 1;
  }
  // A disconnected client must not terminate the process before dirty pages
  // can be flushed during orderly shutdown.
  std::signal(SIGPIPE, SIG_IGN);

  in_addr bindAddressValue{};
  if (::inet_pton(AF_INET, bindAddress.c_str(), &bindAddressValue) != 1) {
    std::cerr << "Invalid IPv4 bind address '" << bindAddress
              << "'. Expected a dotted IPv4 address.\n";
    return 2;
  }

  // Plain SQL is allowed only on a loopback listener. Every
  // non-loopback listener requires mutual TLS, which encrypts traffic and
  // authenticates the connecting client before SQL can be processed.
  const auto hostOrderAddress = ntohl(bindAddressValue.s_addr);
  const bool loopbackAddress = (hostOrderAddress >> 24U) == 127U;
  SslContextPtr tlsContext(nullptr, SSL_CTX_free);
  if (!loopbackAddress) {
    tlsContext = createMutualTlsContext();
    if (!tlsContext)
      return 2;
  }

  const int listener = ::socket(AF_INET, SOCK_STREAM, 0);
  if (listener < 0) {
    std::cerr << "UniDB failed to create TCP socket: " << std::strerror(errno)
              << '\n';
    return 1;
  }

  int reuse = 1;
  setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_port = htons(port);
  address.sin_addr = bindAddressValue;
  if (::bind(listener, reinterpret_cast<sockaddr *>(&address),
             sizeof(address)) != 0 ||
      ::listen(listener, 16) != 0) {
    std::cerr << "UniDB failed to listen on " << bindAddress << ':' << port
              << ": " << std::strerror(errno) << '\n';
    ::close(listener);
    return 1;
  }

  std::cout << "UniDB SQL protocol listening on " << bindAddress << ':' << port
            << '\n';
  // Notify the launcher only after bind() and listen() have both succeeded.
  const char *readyFdValue = std::getenv("UNIDB_READY_FD");
  if (readyFdValue != nullptr) {
    char *end = nullptr;
    errno = 0;
    const long readyFd = std::strtol(readyFdValue, &end, 10);
    if (errno == 0 && end != readyFdValue && *end == '\0' && readyFd >= 0 &&
        readyFd <= std::numeric_limits<int>::max()) {
      const char readySignal = 'R';
      (void)::write(static_cast<int>(readyFd), &readySignal, 1);
      ::close(static_cast<int>(readyFd));
      ::unsetenv("UNIDB_READY_FD");
    }
  }
  constexpr std::size_t maximumConcurrentSessions = 64;
  bool acceptFailed = false;
  std::mutex sessionsMutex;
  std::condition_variable sessionsReady;
  std::deque<ClientConnection> readySessions;
  std::vector<int> activeClientSockets;
  std::size_t activeSessionCount = 0;
  std::mutex containerMutex;
  bool acceptLoopFinished = false;

  std::thread acceptThread;
  try {
    acceptThread =
        std::thread(acceptAndAuthenticateClients, listener, tlsContext.get(),
                    std::ref(sessionsMutex), std::ref(sessionsReady),
                    std::ref(readySessions), std::ref(acceptFailed),
                    std::ref(acceptLoopFinished));
  } catch (...) {
    std::cerr << "UniDB failed to start its connection accept loop.\n";
    ::close(listener);
    return 1;
  }

  while (true) {
    ClientConnection connection{-1, nullptr};
    {
      std::unique_lock<std::mutex> lock(sessionsMutex);
      sessionsReady.wait_for(lock, std::chrono::milliseconds(100), [&]() {
        return !readySessions.empty() || acceptLoopFinished || stopRequested;
      });
      if (stopRequested || acceptLoopFinished)
        break;
      if (!readySessions.empty()) {
        connection = readySessions.front();
        readySessions.pop_front();
        if (activeSessionCount >= maximumConcurrentSessions) {
          std::cerr << "Rejected SQL client: maximum concurrent session limit "
                       "reached.\n";
          closeClientConnection(connection);
          continue;
        }
        activeClientSockets.push_back(connection.socket);
        ++activeSessionCount;
      } else {
        continue;
      }
    }

    // Each authenticated client gets an independent network worker. Only the
    // query action is guarded by containerMutex, so an idle socket cannot hold
    // up another client's SQL while database mutations remain serialized.
    try {
      std::thread clientThread([&, connection]() {
        serveClient(connection.socket, container, connection.tls,
                    containerMutex);
        if (connection.tls != nullptr) {
          (void)SSL_shutdown(connection.tls);
          SSL_free(connection.tls);
        }

        {
          std::lock_guard<std::mutex> lock(sessionsMutex);
          const auto active =
              std::find(activeClientSockets.begin(), activeClientSockets.end(),
                        connection.socket);
          if (active != activeClientSockets.end())
            activeClientSockets.erase(active);
          --activeSessionCount;
          ::close(connection.socket);
        }
        sessionsReady.notify_all();
      });
      clientThread.detach();
    } catch (...) {
      std::cerr << "Could not start SQL client worker.\n";
      closeClientConnection(connection);
      {
        std::lock_guard<std::mutex> lock(sessionsMutex);
        const auto active =
            std::find(activeClientSockets.begin(), activeClientSockets.end(),
                      connection.socket);
        if (active != activeClientSockets.end())
          activeClientSockets.erase(active);
        --activeSessionCount;
      }
      sessionsReady.notify_all();
    }
  }

  // Interrupt all blocking client reads/writes before waiting for workers to
  // finish. Worker threads own SSL objects and close their sockets themselves.
  {
    std::lock_guard<std::mutex> lock(sessionsMutex);
    for (const int client : activeClientSockets)
      (void)::shutdown(client, SHUT_RDWR);
  }

  if (acceptThread.joinable())
    acceptThread.join();
  {
    std::unique_lock<std::mutex> lock(sessionsMutex);
    sessionsReady.wait(lock, [&]() { return activeSessionCount == 0; });
  }
  ::close(listener);
  std::cout << "UniDB server shutting down; flushing dirty pages.\n";
  container.flushDirtyPages();
  std::cout << "UniDB server shutdown complete.\n";
  return acceptFailed ? 1 : 0;
}

} // namespace ConnectionProtocol
