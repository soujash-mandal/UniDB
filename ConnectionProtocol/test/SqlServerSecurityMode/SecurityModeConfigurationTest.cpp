#include <gtest/gtest.h>

#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <csignal>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <thread>

namespace {

int runServerWithMode(const char *mode, const char *bindAddress = "127.0.0.1") {
  char path[] = "/tmp/unidb-security-mode-test-XXXXXX";
  const int tempFd = ::mkstemp(path);
  if (tempFd < 0)
    return -1;
  ::close(tempFd);
  std::filesystem::remove(path);

  const pid_t child = ::fork();
  if (child < 0) {
    std::filesystem::remove(path);
    return -1;
  }
  if (child == 0) {
    ::setenv("UNIDB_SECURITY_MODE", mode, 1);
    ::setenv("UNIDB_BIND_ADDRESS", bindAddress, 1);
    ::unsetenv("UNIDB_DEV_ALLOW_NON_LOOPBACK_BIND");
    ::unsetenv("UNIDB_TLS_CERT_FILE");
    ::unsetenv("UNIDB_TLS_KEY_FILE");
    ::unsetenv("UNIDB_TLS_CA_FILE");
    const auto executable = std::filesystem::current_path() / "unidb";
    ::execl(executable.c_str(), executable.c_str(), "--server", "--db-path",
            path, "--port", "17432", static_cast<char *>(nullptr));
    _exit(127);
  }

  int status = 0;
  bool reaped = false;
  for (int attempt = 0; attempt < 100; ++attempt) {
    const pid_t result = ::waitpid(child, &status, WNOHANG);
    if (result == child) {
      reaped = true;
      break;
    }
    if (result < 0 && errno != EINTR)
      break;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  if (!reaped) {
    ::kill(child, SIGKILL);
    while (::waitpid(child, &status, 0) < 0 && errno == EINTR) {
    }
  }

  std::filesystem::remove(path);
  return reaped && WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

TEST(SqlServerSecurityModeTest, ProductionModeRequiresMutualTlsOnLoopback) {
  EXPECT_EQ(runServerWithMode("production"), 2);
}

TEST(SqlServerSecurityModeTest, DevelopmentModeRejectsNonLoopbackWithoutOptIn) {
  EXPECT_EQ(runServerWithMode("development", "0.0.0.0"), 2);
}

TEST(SqlServerSecurityModeTest, RejectsUnknownSecurityMode) {
  EXPECT_EQ(runServerWithMode("insecure"), 2);
}

} // namespace
