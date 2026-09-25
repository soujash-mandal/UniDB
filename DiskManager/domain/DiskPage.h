#pragma once

#include <cstddef>

class DiskPage {
public:
  static constexpr std::size_t PAGE_SIZE = 8192;
  DiskPage() = default;
  char *data() { return bytes; }
  const char *data() const { return bytes; }

private:
  char bytes[PAGE_SIZE]{};
};