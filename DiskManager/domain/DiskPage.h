#pragma once

#include <cstddef>
#include <cstdint>

using DiskPageId = uint32_t;

class DiskPage {
public:
  static constexpr std::size_t PAGE_SIZE = 8192;
  DiskPage() = default;
  char *data() { return bytes; }
  const char *data() const { return bytes; }

private:
  char bytes[PAGE_SIZE]{};
};