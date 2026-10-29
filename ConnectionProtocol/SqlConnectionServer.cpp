#include "SqlConnectionServer.h"

#include "../QueryExecutionService/actions/ExecuteQueryAction.h"
#include "../container/container.h"
#include "SqlRequestFramer.h"

#include <algorithm>
#include <cerrno>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
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

void serveClient(int client, Container &container, SSL *tls) {
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
      const auto result = container.executeQueryAction().execute(sql);
      if (!sendAll(client, tls, responseJson(result)))
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
  bool acceptFailed = false;
  while (!stopRequested) {
    const int client = ::accept(listener, nullptr, nullptr);
    if (client < 0) {
      if (errno == EINTR && stopRequested)
        break;
      if (errno == EINTR || errno == ECONNABORTED)
        continue;
      std::cerr << "UniDB accept failed: " << std::strerror(errno) << '\n';
      acceptFailed = true;
      break;
    }
    SSL *tls = nullptr;
    if (tlsContext) {
      tls = SSL_new(tlsContext.get());
      if (tls == nullptr || SSL_set_fd(tls, client) != 1) {
        std::cerr << "Could not initialize TLS for an incoming SQL client.\n";
        ERR_print_errors_fp(stderr);
        if (tls != nullptr)
          SSL_free(tls);
        ::close(client);
        continue;
      }

      // Bound unauthenticated handshake time so a stalled client cannot hold
      // the single-threaded server before presenting its certificate.
      timeval handshakeTimeout{};
      handshakeTimeout.tv_sec = 5;
      (void)::setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &handshakeTimeout,
                         sizeof(handshakeTimeout));
      (void)::setsockopt(client, SOL_SOCKET, SO_SNDTIMEO, &handshakeTimeout,
                         sizeof(handshakeTimeout));
      const int handshakeResult = SSL_accept(tls);
      timeval noTimeout{};
      (void)::setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &noTimeout,
                         sizeof(noTimeout));
      (void)::setsockopt(client, SOL_SOCKET, SO_SNDTIMEO, &noTimeout,
                         sizeof(noTimeout));
      if (handshakeResult != 1) {
        std::cerr << "Rejected SQL client: mutual TLS authentication failed.\n";
        ERR_clear_error();
        SSL_free(tls);
        ::close(client);
        continue;
      }
    }

    serveClient(client, container, tls);
    if (tls != nullptr) {
      (void)SSL_shutdown(tls);
      SSL_free(tls);
    }
    ::close(client);
  }

  ::close(listener);
  std::cout << "UniDB server shutting down; flushing dirty pages.\n";
  container.flushDirtyPages();
  std::cout << "UniDB server shutdown complete.\n";
  return acceptFailed ? 1 : 0;
}

} // namespace ConnectionProtocol
