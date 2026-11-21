#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

// WAL-owned abstraction for byte-addressable log storage.
// flush() drains the adapter's stream buffer; it does not promise an OS-level
// fsync or stable-storage guarantee.
class LogStoragePort {
public:
  virtual ~LogStoragePort() = default;

  virtual void writeAt(uint64_t offset,
                       const std::vector<std::byte> &bytes) = 0;
  virtual std::vector<std::byte> readAt(uint64_t offset,
                                        std::size_t byteCount) = 0;
  virtual void flush() = 0;
};
