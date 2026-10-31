#include <gtest/gtest.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

namespace {

class SqlServerSocketIntegrationTest : public ::testing::Test {
protected:
  // Reserve an unused loopback TCP port for the child server.
  static unsigned short reserveEphemeralPort() {
    const int socketFd = ::socket(AF_INET, SOCK_STREAM, 0);
    EXPECT_GE(socketFd, 0);
    if (socketFd < 0)
      return 0;

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = 0;

    EXPECT_EQ(::bind(socketFd, reinterpret_cast<sockaddr *>(&address),
                     sizeof(address)),
              0);
    socklen_t length = sizeof(address);
    EXPECT_EQ(::getsockname(socketFd, reinterpret_cast<sockaddr *>(&address),
                            &length),
              0);
    const auto port = ntohs(address.sin_port);
    ::close(socketFd);
    return port;
  }

  // Connect to the test server, returning -1 when it is not ready.
  static int connectToServer(unsigned short port) {
    const int socketFd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (socketFd < 0)
      return -1;

#ifdef SO_NOSIGPIPE
    // Prevent a peer closing the connection from terminating the test process.
    const int noSigPipe = 1;
    if (::setsockopt(socketFd, SOL_SOCKET, SO_NOSIGPIPE, &noSigPipe,
                     sizeof(noSigPipe)) != 0) {
      ::close(socketFd);
      return -1;
    }
#endif

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (::connect(socketFd, reinterpret_cast<sockaddr *>(&address),
                  sizeof(address)) != 0) {
      ::close(socketFd);
      return -1;
    }
    return socketFd;
  }

  // Send the complete request, retrying interrupted system calls.
  static bool sendAll(int socketFd, const std::string &request) {
    std::size_t sent = 0;
    while (sent < request.size()) {
      int sendFlags = 0;
#ifdef MSG_NOSIGNAL
      sendFlags |= MSG_NOSIGNAL;
#endif
      const auto count = ::send(socketFd, request.data() + sent,
                                request.size() - sent, sendFlags);
      if (count < 0) {
        if (errno == EINTR)
          continue;
        return false;
      }
      if (count == 0)
        return false;
      sent += static_cast<std::size_t>(count);
    }
    return true;
  }

  // Read one newline-terminated response, with a five-second receive deadline.
  // Return an empty string for timeout, read error, or an incomplete frame.
  static std::string readResponse(int socketFd) {
    timeval timeout{};
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    if (::setsockopt(socketFd, SOL_SOCKET, SO_RCVTIMEO, &timeout,
                     sizeof(timeout)) != 0)
      return {};

    std::string response;
    char ch = '\0';
    while (response.size() < 1024 * 1024) {
      const auto count = ::recv(socketFd, &ch, 1, 0);
      if (count == 0)
        return {};
      if (count < 0) {
        if (errno == EINTR)
          continue;
        return {};
      }
      response += ch;
      if (ch == '\n')
        return response;
    }
    return {};
  }

  void SetUp() override {
    char path[] = "/tmp/unidb-sql-socket-test-XXXXXX";
    const int tempFd = ::mkstemp(path);
    ASSERT_GE(tempFd, 0);
    ::close(tempFd);
    databasePath = path;
    std::filesystem::remove(databasePath);

    port = reserveEphemeralPort();
    ASSERT_NE(port, 0);

    const auto executable = std::filesystem::current_path() / "unidb";
    serverPid = ::fork();
    ASSERT_GE(serverPid, 0);
    if (serverPid == 0) {
      ::setenv("UNIDB_SECURITY_MODE", "development", 1);
      ::setenv("UNIDB_BIND_ADDRESS", "127.0.0.1", 1);
      const auto portText = std::to_string(port);
      ::execl(executable.c_str(), executable.c_str(), "--server", "--db-path",
              databasePath.c_str(), "--port", portText.c_str(),
              static_cast<char *>(nullptr));
      _exit(127);
    }

    for (int attempt = 0; attempt < 100; ++attempt) {
      const int socketFd = connectToServer(port);
      if (socketFd >= 0) {
        ::close(socketFd);
        return;
      }
      int status = 0;
      if (::waitpid(serverPid, &status, WNOHANG) == serverPid) {
        serverPid = -1;
        FAIL() << "UniDB server exited before accepting connections";
        return;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    FAIL() << "UniDB server did not become ready within five seconds";
  }

  void TearDown() override {
    if (serverPid > 0) {
      // Allow graceful shutdown to flush dirty pages, but never hang the suite.
      EXPECT_EQ(::kill(serverPid, SIGTERM), 0);
      int status = 0;
      bool reaped = false;
      for (int attempt = 0; attempt < 100; ++attempt) {
        const pid_t result = ::waitpid(serverPid, &status, WNOHANG);
        if (result == serverPid) {
          reaped = true;
          break;
        }
        if (result < 0 && errno != EINTR) {
          ADD_FAILURE() << "waitpid failed while awaiting server shutdown";
          break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
      }

      if (!reaped) {
        ADD_FAILURE() << "UniDB server did not shut down within five seconds";
        ::kill(serverPid, SIGKILL);
        while (::waitpid(serverPid, &status, 0) < 0 && errno == EINTR) {
        }
      } else {
        EXPECT_TRUE(WIFEXITED(status));
        if (WIFEXITED(status))
          EXPECT_EQ(WEXITSTATUS(status), 0);
      }
    }
    std::error_code error;
    std::filesystem::remove(databasePath, error);
    std::filesystem::remove(databasePath + ".pid", error);
    std::filesystem::remove(databasePath + ".log", error);
    std::filesystem::remove(databasePath + ".lifecycle.lock", error);
  }

  std::string databasePath;
  unsigned short port = 0;
  pid_t serverPid = -1;
};

TEST_F(SqlServerSocketIntegrationTest,
       ExecutesCreateInsertAndSelectOverTcpConnection) {
  // 1.1 Connect to the foreground server through its documented TCP protocol.
  const int socketFd = connectToServer(port);
  ASSERT_GE(socketFd, 0);

  // 1.2 Verify a command response is returned as one newline-delimited JSON
  // frame.
  ASSERT_TRUE(
      sendAll(socketFd, "CREATE TABLE socket_test (id INT, name TEXT);\n"));
  const auto createResponse = readResponse(socketFd);
  EXPECT_NE(createResponse.find("\"ok\":true"), std::string::npos);

  // 1.3 Send another request on the same connection and verify insertion.
  ASSERT_TRUE(sendAll(
      socketFd, "INSERT INTO socket_test (id, name) VALUES (7, 'Ada');\n"));
  const auto insertResponse = readResponse(socketFd);
  EXPECT_NE(insertResponse.find("\"ok\":true"), std::string::npos);
  EXPECT_NE(insertResponse.find("\"affected_rows\":1"), std::string::npos);

  // 1.4 Verify SELECT returns the row inserted earlier on this TCP connection.
  ASSERT_TRUE(sendAll(socketFd, "SELECT * FROM socket_test;\n"));
  const auto selectResponse = readResponse(socketFd);
  EXPECT_NE(selectResponse.find("\"kind\":\"rows\""), std::string::npos);
  EXPECT_NE(selectResponse.find("\"rows\":[[7,\"Ada\"]]"), std::string::npos);
  ::close(socketFd);
}

} // namespace
