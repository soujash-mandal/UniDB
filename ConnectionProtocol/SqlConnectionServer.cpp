#include "SqlConnectionServer.h"

#include "../QueryExecutionService/actions/ExecuteQueryAction.h"
#include "../container/container.h"
#include "SqlRequestFramer.h"

#include <cerrno>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

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

bool sendAll(int socket, std::string_view bytes) {
  while (!bytes.empty()) {
    if (stopRequested)
      return false;
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

void serveClient(int client, Container &container) {
  SqlRequestFramer framer;
  char buffer[4096];
  while (!stopRequested) {
    const auto received = ::recv(client, buffer, sizeof(buffer), 0);
    if (received == 0)
      break;
    if (received < 0) {
      if (errno == EINTR)
        continue;
      break;
    }

    auto framing = framer.append(
        std::string_view(buffer, static_cast<std::size_t>(received)));
    for (const auto &sql : framing.requests) {
      const auto result = container.executeQueryAction().execute(sql);
      if (!sendAll(client, responseJson(result)))
        return;
    }
    if (framing.error) {
      const std::string response =
          "{\"ok\":false,\"error\":{\"code\":\"invalid_request\",\"message\":" +
          jsonString(*framing.error) + "}}\n";
      sendAll(client, response);
      break;
    }
  }
}
} // namespace

SqlConnectionServer::SqlConnectionServer(Container &container,
                                         std::uint16_t port,
                                         std::string bindAddress)
    : container(container), port(port), bindAddress(std::move(bindAddress)) {
  if (this->bindAddress.empty()) {
    const char *configuredAddress = std::getenv("UNIDB_BIND_ADDRESS");
    this->bindAddress =
        configuredAddress == nullptr ? "127.0.0.1" : configuredAddress;
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

  // SQL traffic is currently plaintext and unauthenticated. Refuse remote
  // listeners unless the operator explicitly opts into that risk.
  const auto hostOrderAddress = ntohl(bindAddressValue.s_addr);
  const bool loopbackAddress = (hostOrderAddress >> 24U) == 127U;
  const char *allowInsecureRemote =
      std::getenv("UNIDB_ALLOW_INSECURE_REMOTE_ACCESS");
  if (!loopbackAddress &&
      (allowInsecureRemote == nullptr ||
       std::string_view(allowInsecureRemote) != "1")) {
    std::cerr << "Refusing non-loopback SQL bind without TLS/authentication. "
                 "Use loopback or explicitly set "
                 "UNIDB_ALLOW_INSECURE_REMOTE_ACCESS=1 only on a trusted, "
                 "isolated network.\n";
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
    serveClient(client, container);
    ::close(client);
  }

  ::close(listener);
  std::cout << "UniDB server shutting down; flushing dirty pages.\n";
  container.flushDirtyPages();
  std::cout << "UniDB server shutdown complete.\n";
  return acceptFailed ? 1 : 0;
}

} // namespace ConnectionProtocol
