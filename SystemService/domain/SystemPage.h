#pragma once

#include <cstddef>
#include <string>

#include "../domain/type.h"

class SystemPage {
public:
  static constexpr std::size_t PAGE_SIZE = 8192;

  SystemPage();

  char *data() { return bytes; }
  const char *data() const { return bytes; }

  void set(std::string key, SystemPageId pageId);
  SystemPageId get(std::string key);

  bool contains(std::string key);
  void remove(std::string key);

private:
  char bytes[PAGE_SIZE]{};
};
