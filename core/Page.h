#pragma once

#include <cstddef>
#include <cstdint>

using PageId = uint32_t;
class Page {
public:
  static constexpr std::size_t PAGE_SIZE = 8192;
  Page() = default;
  char *data() { return bytes; }
  const char *data() const { return bytes; }

private:
  char bytes[PAGE_SIZE]{};
};